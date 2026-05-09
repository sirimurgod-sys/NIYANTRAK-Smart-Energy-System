// ============================================
// NIYANTRAK - FINAL VERSION (v3.2)
// Smart Residential Energy Optimization System
// Fixed: AC Relay cut now reflects in load calc
// ============================================

#define POT_PIN 34
#define RELAY_AC 2
#define FAN_PIN 4
#define HEATER_PIN 5
#define STATUS_LED 18

// Thresholds
float totalLoad = 0;
float threshold = 2000;
float warningLevel = 1500;

// Device states
int fanSpeed = 100;
int heaterLevel = 100;
int acTemp = 24;
bool acRelayCut = false;

// Failover
bool primaryActive = true;
unsigned long startTime;

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(115200);

  pinMode(RELAY_AC, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);

  digitalWrite(RELAY_AC, HIGH);
  digitalWrite(STATUS_LED, LOW);
  acRelayCut = false;

  startTime = millis();

  Serial.println("\n===== NIYANTRAK SYSTEM STARTED =====");
}

// ============================================
// MAIN LOOP
// ============================================
void loop() {

  // Startup stabilization
  if (millis() - startTime < 3000) {
    Serial.println("Initializing system...");
    delay(1000);
    return;
  }

  // Controller status first
  Serial.println("\n-----------------------------");
  Serial.print("Controller: ");
  Serial.println(primaryActive ? "PRIMARY" : "BACKUP");

  // Read load
  int sensorValue = analogRead(POT_PIN);
  totalLoad = map(sensorValue, 0, 4095, 300, 3000);

  Serial.print("Measured Load: ");
  Serial.print(totalLoad);
  Serial.println(" W");

  // Reset relay cut flag when load is back to normal
  // so system can restore AC in next normal cycle
  if (totalLoad < warningLevel && acRelayCut) {
    acRelayCut = false;
    digitalWrite(RELAY_AC, HIGH);
    Serial.println("✔ AC Relay Restored — Load Normal");
  }

  // State decision
  if (totalLoad < warningLevel) {
    normalState();
  }
  else if (totalLoad < threshold) {
    warningState();
  }
  else {
    criticalState();
  }

  // Failover simulation (triggers once after 25 sec)
  if (millis() - startTime > 25000 && primaryActive) {
    primaryActive = false;
    Serial.println("\n⚠️  Primary Controller Failed!");
    Serial.println("🔁 Backup Controller Activated");
    digitalWrite(STATUS_LED, HIGH);
  }

  delay(1000);
}

// ============================================
// NORMAL STATE
// ============================================
void normalState() {
  Serial.println("State: NORMAL");

  fanSpeed = 100;
  heaterLevel = 100;
  acTemp = 24;

  if (!acRelayCut) {
    digitalWrite(RELAY_AC, HIGH);
  }

  applyControl();
}

// ============================================
// WARNING STATE
// ============================================
void warningState() {
  Serial.println("State: WARNING (Proactive)");

  fanSpeed = 85;
  heaterLevel = 80;
  acTemp = 26;

  if (!acRelayCut) {
    digitalWrite(RELAY_AC, HIGH);
  }

  applyControl();

  float estimatedLoad = calculateLoad();
  printAdjustedLoad(estimatedLoad);

  if (estimatedLoad < warningLevel) {
    Serial.println("✔ Load returned to NORMAL range");
  } else {
    Serial.println("⚠ Monitoring continues...");
  }
}

// ============================================
// CRITICAL STATE (MULTI-STAGE)
// ============================================
void criticalState() {
  Serial.println("State: CRITICAL (Adaptive Control)");

  // Start from current calculated load
  float estimatedLoad = calculateLoad();
  int stage = 1;

  while (estimatedLoad > threshold) {

    // Safety exit — prevents infinite loop
    if (stage > 10) {
      Serial.println("⚠ Maximum optimization reached");
      Serial.println("⚠ Manual intervention may be needed");
      break;
    }

    Serial.print("\nStage ");
    Serial.println(stage);

    // Priority order:
    // 1. Heater (highest load consumer)
    // 2. Fan speed
    // 3. AC temperature
    // 4. Cut AC relay completely (last resort)

    if (heaterLevel > 0) {
      heaterLevel -= 20;
      if (heaterLevel < 0) heaterLevel = 0;
      Serial.println("→ Reducing Heater");
    }
    else if (fanSpeed > 40) {
      fanSpeed -= 10;
      Serial.println("→ Reducing Fan Speed");
    }
    else if (acTemp < 32) {
      acTemp += 1;
      Serial.println("→ Raising AC Temperature");
    }
    else if (!acRelayCut) {
      // FIX: Only cut relay once, set flag
      acRelayCut = true;
      digitalWrite(RELAY_AC, LOW);
      Serial.println("→ AC Relay CUT (Last Resort)");
    }
    else {
      // All options exhausted
      Serial.println("→ All optimizations applied");
    }

    applyControl();

    // FIX: calculateLoad now accounts for relay cut
    estimatedLoad = calculateLoad();
    printAdjustedLoad(estimatedLoad);

    delay(500);
    stage++;
  }

  if (estimatedLoad <= threshold) {
    Serial.println("\n✔ Load Stabilized Within Safe Limits");
  }
}

// ============================================
// LOAD ESTIMATION
// FIX: Added acRelayCut flag — 800W reduction
// when AC relay is physically cut
// ============================================
float calculateLoad() {
  float reduction =
    (100 - heaterLevel) * 5 +
    (100 - fanSpeed) * 2 +
    (acTemp - 24) * 20 +
    (acRelayCut ? 800 : 0);  // AC cut = ~800W saved

  float adjusted = totalLoad - reduction;
  if (adjusted < 0) adjusted = 0;

  return adjusted;
}

// ============================================
// APPLY CONTROL TO DEVICES
// ============================================
void applyControl() {
  analogWrite(FAN_PIN, map(fanSpeed, 0, 100, 0, 255));
  analogWrite(HEATER_PIN, map(heaterLevel, 0, 100, 0, 255));
  sendZigbeeCommand();

  Serial.print("Fan:    "); Serial.print(fanSpeed);    Serial.println("%");
  Serial.print("Heater: "); Serial.print(heaterLevel); Serial.println("%");
  Serial.print("AC:     "); Serial.print(acTemp);      Serial.println("°C");
  Serial.print("Relay:  "); Serial.println(acRelayCut ? "OFF" : "ON");
}

// ============================================
// PRINT ADJUSTED LOAD
// ============================================
void printAdjustedLoad(float val) {
  Serial.print("Estimated Load: ");
  Serial.print(val);
  Serial.println(" W");
}

// ============================================
// ZIGBEE COMMAND SIMULATION
// ============================================
void sendZigbeeCommand() {
  Serial.println("📡 Zigbee: Control Signal Sent");
}
