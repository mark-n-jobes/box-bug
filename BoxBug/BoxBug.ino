//------------------------------------------------------------------------//
#define FixedPntScaler 128 // For range expansion
#define CountFactor 11     // Bigger->slower(but more charge time sums)
String inStringRaw = "";
boolean Armed = false;
const byte PinLocations[3] = {2,9,5};
unsigned long PinTimes[3]  = {0}; // {0,1,2} = {LeftWall,Floor,RightWall}
unsigned long PinTimesOffset[3] = {0};
//------------------------------------------------------------------------//
void setup() {
    Serial.begin(115200);
    Serial.println("Started Box Interface");
    // Setup pins for IR LEDs (hooked up to A0-A3 w/GND for GND)
    DDRC |= B1111; // Remember this is for bursting 4
    // Init all other pins to output and low
    DDRB |= B11111;
    PINB = 0;
    DDRD |= B11111111;
    PIND = 0;
}
//------------------------------------------------------------------------//
void loop() {
    MeasureChargeTimes(PinTimes,PinLocations,3);
    if(Armed) {
        SendInfo();
        // Test for Bug Movement (and move if needed)
        long DeltaLeft  = (long)PinTimes[0]-(long)PinTimesOffset[0];
        long DeltaFloor = (long)PinTimes[1]-(long)PinTimesOffset[1];
        long DeltaRight = (long)PinTimes[2]-(long)PinTimesOffset[2];
        byte MaxID = ((DeltaLeft > DeltaFloor)&&(DeltaLeft > DeltaRight))? 0:
                                               (DeltaFloor > DeltaRight) ? 1:
                                                                           2;
        if((DeltaLeft > 20)&&(MaxID==0)) {
            Xmit_InchCode('F');
            Serial.println("Move Forward");
        } else if((DeltaFloor > 20)&&(MaxID==1)) {
            Xmit_InchCode('B');
            Serial.println("Move Backward");
        } else if((DeltaRight > 20)&&(MaxID==2)) {
            Xmit_InchCode('R');
            Serial.println("Move Right");
        }
    } else {
        for(byte vidx=0;vidx<3;vidx++) {
            PinTimesOffset[vidx] += PinTimes[vidx];
            PinTimesOffset[vidx] >>= 1;
        }
    }
}
//------------------------------------------------------------------------//
void serialEvent() {
    char inChar = Serial.read();
    if((inChar >= 'A')&&(inChar <= 'z')) inStringRaw += (char)inChar;
    else {
        if(inStringRaw.equals("Arm")) {
            Serial.println("Armed!");
            Armed = true;
        } else if(inStringRaw.equals("Unarm")) {
            Serial.println("Unarmed!");
            Armed = false;
        }
        inStringRaw = "";
    }
}
//------------------------------------------------------------------------//
void SendInfo() {
    for(byte vidx=0;vidx<3;vidx++) {
        long temp = PinTimes[vidx]-PinTimesOffset[vidx]; // Grab value and subtract offset
        Serial.print((temp>>1)<<1); // Shift back-and-forth to ignore LSB flips
        Serial.print("\t");
    }
    Serial.println(" ");
}
//------------------------------------------------------------------------//
void MeasureChargeTimes(unsigned long DataVector[], const byte Pins[], const byte numPins) {
    for(byte vidx=0;vidx<numPins;vidx++) {
        unsigned long sum = 0;  // Sum over multiple charge/discharge cycles
        byte PinID = Pins[vidx];
        for(long i=0;i<(1<<CountFactor);i++) {
            // Clear charge on only the one we measure
            pinMode(PinID, OUTPUT);
            digitalWrite(PinID, LOW);
            // Prepare to measure charge
            pinMode(PinID, INPUT);
            // Measure (using port ref for speed)
            if(PinID < 8) while((PIND & (1<< PinID))    == 0) sum++;
            else          while((PINB & (1<<(PinID&7))) == 0) sum++;
        }
        DataVector[vidx] = (unsigned long)sqrt(sum*FixedPntScaler); // Scaler for range enhance,
    }                                                               // and sqrt to linearize range
}
//------------------------------------------------------------------------//
void Xmit_InchCode(char Code) {
    int CodeArray[1] = {0};
    if      (Code == 'F') CodeArray[0] = 131;
    else if (Code == 'B') CodeArray[0] = 69; // heheh
    else if (Code == 'R') CodeArray[0] = 38;
    else if (Code == 'L') CodeArray[0] = 193;
    // Header
    pulseIR_38KHz(4000); delayMicroseconds(400);
    pulseIR_38KHz(2000); delayMicroseconds(800);
    // Body
    sendCharArray(CodeArray,1,400,1400,500);
    // Footer
    pulseIR_38KHz(400); delayMicroseconds(1400);
    pulseIR_38KHz(400); delayMicroseconds(1400);
    pulseIR_38KHz(400);
    delay(65);
}
//------------------------------------------------------------------------//
void sendCharArray(int CharArray[], uint16_t size, uint16_t PulseOn, uint16_t DelayTrue, uint16_t DelayFalse) {
    int tempc = 0;
    for(int i=0;i < size;i++) {
        tempc = CharArray[i];
        for(int j=7;j>=0;j--) {
            pulseIR_38KHz(PulseOn);
            if (((tempc>>j)%2) == 1) delayMicroseconds(DelayTrue);
            else                     delayMicroseconds(DelayFalse);
        }
    }
}
//------------------------------------------------------------------------//
void pulseIR_38KHz(long microsecs) {
    while (microsecs > 0) {
        // 38KHz = 26.316us -> 26us, so cut in 1/2 for IR pulse -> 13us high, 13us low
        PORTC = B1111;  // Takes ~1/2us
        delayMicroseconds(13);
        PORTC = B0000;  // Takes ~1/2us
        delayMicroseconds(12);
        microsecs -= 26;
    }
}
//------------------------------------------------------------------------//