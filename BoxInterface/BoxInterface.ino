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
    // Note we can skip pin modes since the measuring function handles such
    // Init pins to output and low
    for(byte i=0;i<14;i++) { // Leave 13 alone
        pinMode(i,OUTPUT);
    }
    PINB = 0;
    PIND = 0;
}
//------------------------------------------------------------------------//
void loop() {
    MeasureChargeTimes(PinTimes,PinLocations,3);
    if(Armed) {
        for(byte vidx=0;vidx<3;vidx++) {
            long temp = PinTimes[vidx]-PinTimesOffset[vidx]; // Grab value and subtract offset
            Serial.print((temp>>1)<<1); // Shift back-and-forth to ignore LSB errors
            Serial.print("\t");
        }
        Serial.println(" ");
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
