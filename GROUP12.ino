#include <AceRoutine.h>
#include <RF24.h>
using namespace ace_routine;


RF24 radio(7, 8);
const byte packetAddress[6] = "bcast";
unsigned char packet[32] = {0x50, 0x0C}; 


// CLASS FOR MANAGAGING ALL NODES
class NodeManager {
private:
    unsigned char nodeAddresses[32][32] = {{NULL}};   //KEEP NODE ADDRESSES
    // First field is each address, second field in neighbours of that address
    unsigned long latestUpdateTime[32] = {0};

public:
    void updateNodes(unsigned char address[32]) {  //UPDATE NODE ADDRESSES
        for (int i = 0; i < 32; i++) {
            if (address [1] == nodeAddresses[i][1]) {
                latestUpdateTime[i] = millis();
            }
        }
    }

    int hasNeighbours(unsigned char address[32]) {    //CHECK FOR NEIGHBOURS
        for (int i = 0; i < 32; i++) {
            if (address[1] == nodeAddresses[i][1]) {
                return 1;
            }
        }
        return 0;
    }


      void addNeighbours(unsigned char address[32]) {   //ADD NEIGHBOURS
        for (int i = 0; i < 32; i++) {
            if (nodeAddresses[i][1] == NULL) {
                nodeAddresses[i][1] = address[1];
                //Skip type, x, source address, and length fields
                packet[i+3] = address[1];
                latestUpdateTime[i] = millis();
                //Update the length field
                packet[2] = sizeof(packet-3);
                addNeighboursNeighbours(address, i);
                return;
            }
        }
    }

    void addNeighboursNeighbours(unsigned char address[32], int i) {
      for(int k = 3; k < 27; k++) {
        nodeAddresses[i][k] = address[k];
      }
      }

    void refreshneighbours() {        //REFRESH NEIGHBOURS
        unsigned long currentTime = millis();
        for (int i = 0; i < 32; i++) {
            if (currentTime - latestUpdateTime[i] > 6000) {
                latestUpdateTime[i] = 0;
                for (int j = 0; j < 32; j++) {
                  nodeAddresses[i][j] = 0;
                }
                packet[i+3] = 0;
            }
        }
    }

    void printneighbours() {     //PRINT OUT NEIGHBOURS
    Serial.print("...................................... ");
        Serial.print("NEIGHBOURING NODES: ");
        for (int i = 0; i < 32; i++) {
            if (nodeAddresses[i] != NULL) {
                Serial.print(nodeAddresses[i][1]);
                Serial.print(" - ");
                for (int j = 0; j < 27; j++) {
                  Serial.print(nodeAddresses[i][j]);
                  Serial.print(" ");
                }
            }
            Serial.print("\n");
        }
        Serial.print("Updated Nodes");
    }
};


class HelloMonitor : public Coroutine {   //MANAGES LISTETING TO HELLO MESSAGE FROM OTHER NODES
public:
    int runCoroutine() override {
        COROUTINE_LOOP() {
            COROUTINE_DELAY(random(1000, 5000));
            radio.stopListening();
            radio.write(packet, sizeof(packet));
            radio.startListening();
        }
    }
};

class NeighbourListener : public Coroutine {
private:
   NodeManager* nodeManager;
public:
    NeighbourListener(NodeManager* noder) {
        nodeManager = noder;
    }
    unsigned char input[33];
    int runCoroutine() override {
        COROUTINE_LOOP() {
            if (radio.available()) {
                radio.read(input, 32);
                if ((input[0] >> 4) == 5) {
                       if (nodeManager->hasNeighbours(input[1])) {  
                        nodeManager->updateNodes(input[1]);
                        //Need to add the neighbours of the incoming address        
                    } else {
                        nodeManager->addNeighbours(input);
                        //Need to add the neighbours of the incoming address  
  
                    }
                }
            }
            COROUTINE_YIELD();
        }
    }
};

class Refresh : public Coroutine {
private:
  NodeManager* nodeManager;
public:
    Refresh(NodeManager* noder) {
        nodeManager = noder;
    }
    int runCoroutine() override {
        COROUTINE_LOOP() {
            nodeManager->refreshneighbours();
            COROUTINE_DELAY(1000);
        }
    }
};


class PrintNeighbours : public Coroutine {
private:
    NodeManager* nodeManager;
public:
    PrintNeighbours(NodeManager* noder) {
        nodeManager = noder;
    }
    int runCoroutine() override {
        COROUTINE_LOOP() {
            nodeManager->printneighbours();
            COROUTINE_DELAY(1000);
        }
    }
};


NodeManager nodeManager;
HelloMonitor helloCoroutine;
NeighbourListener listenCoroutine(&nodeManager);
Refresh refreshCoroutine(&nodeManager);
PrintNeighbours printCoroutine(&nodeManager);


void setup() {
    Serial.begin(9600);
    if (radio.begin()) {
        Serial.println("Device hardware is working");
    } else {
        Serial.println("Device hardware is not working");
    }
    radio.openReadingPipe(1, packetAddress);
    radio.openWritingPipe(packetAddress);
    radio.setPALevel(RF24_PA_MAX);
    radio.setAutoAck(false);
    radio.startListening();
    CoroutineScheduler::setup();
}


void loop() {
    CoroutineScheduler::loop();
}
