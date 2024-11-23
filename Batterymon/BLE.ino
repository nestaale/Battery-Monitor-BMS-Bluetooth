//  ----- BLE stuff -----
static BLERemoteCharacteristic *pRemoteCharacteristic;
static BLEAdvertisedDevice *myDevice;
BLERemoteService *pRemoteService;
// The remote service we wish to connect to. Needs check/change when other BLE module used.
static BLEUUID serviceUUID("0000ff00-0000-1000-8000-00805f9b34fb"); //xiaoxiang bms original module
static BLEUUID charUUID_tx("0000ff02-0000-1000-8000-00805f9b34fb"); //xiaoxiang bms original module
static BLEUUID charUUID_rx("0000ff01-0000-1000-8000-00805f9b34fb"); //xiaoxiang bms original module

// My Eco-Worthy Batteries:
// Battery 1:
// Device UUID: 98CB4D0B-CEAE-9BC0-EDBCEAD8D844
// Battery 2:
// Device UUID: 92BE824C-EC22-2541-5ACECFD11BCD

// 0000ff01-0000-1000-8000-00805f9b34fb
// NOTIFY, READ
// Notifications from this characteristic is received data from BMS

// 0000ff02-0000-1000-8000-00805f9b34fb
// Write this characteristic to send data to BMS
// READ, WRITE, WRITE NO RESPONSE

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{ //this is called by some underlying magic
    /**
	* Called for each advertising BLE server.
	*/
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        Serial.print("BLE Advertised Device found: ");
        Serial.println(advertisedDevice.toString().c_str());

        // We have found a device, let us now see if it contains the service we are looking for.
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID))
        {

            BLEDevice::getScan()->stop();
            myDevice = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
            doScan = true;

        } // Found our server
    }     // onResult
};        // MyAdvertisedDeviceCallbacks
class MyClientCallback : public BLEClientCallbacks
{ //this is called on connect / disconnect by some underlying magic+

    void onConnect(BLEClient *pclient)
    {
    }

    void onDisconnect(BLEClient *pclient)
    {
        BLE_client_connected = false;
        Serial.println("onDisconnect");
        //lcdDisconnect();
    }
};

void bleRequestData()
{
#ifndef SIMULATION

    // If the flag "doConnect" is true then we have scanned for and found the desired
    // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
    // connected we set the connected flag to be true.
    if (doConnect == true)
    {
        if (connectToServer())
        {
            Serial.println("We are now connected to the BLE Server.");
            //lcdConnected();
        }
        else
        {
            //lcdConnectionFailed();
        }
        doConnect = false;
    }

    // If we are connected to a peer BLE Server, update the characteristic each time we are reached
    // with the current time since boot.
    if (BLE_client_connected == true)
    {

        unsigned long currentMillis = millis();
        if ((currentMillis - previousMillis >= interval || newPacketReceived)) //every time period or when packet is received
        {
            previousMillis = currentMillis;
            showInfoLcd();//UPDATE INFORMATION

            if (toggle) //alternate info3 and info4
            {
                bmsGetInfo3();
                //showBasicInfo();
                newPacketReceived = false;
            }
            else
            {
                bmsGetInfo4();
                //showCellInfo();
                newPacketReceived = false;
            }
            toggle = !toggle;
        }
    }
    else if (doScan)
    {
        BLEDevice::getScan()->start(0); // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
    }
#endif

#ifdef SIMULATION
    bmsSimulate();
#endif
}

void bleStartup()
{
#ifndef SIMULATION

    TRACE;
    BLEDevice::init("");

    // Retrieve a Scanner and set the callback we want to use to be informed when we
    // have detected a new device.  Specify that we want active scanning and start the
    // scan to run for 5 seconds.
    BLEScan *pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, true);
#endif
}

static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) //this is called when BLE server sents data via notofication
{
    TRACE;
    //hexDump((char*)pData, length);
    bleCollectPacket((char *)pData, length);
}

bool connectToServer()
{
    TRACE;
    Serial.print("Forming a connection to ");
    lcdConnectingStatus(0);
    Serial.println(myDevice->getAddress().toString().c_str());
    BLEClient *pClient = BLEDevice::createClient();
    Serial.println(" - Created client");
    lcdConnectingStatus(1);
    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice); // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");
    lcdConnectingStatus(2);
    // Obtain a reference to the service we are after in the remote BLE server.
    //BLERemoteService*
    pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr)
    {
        Serial.print("Failed to find our service UUID: ");
        lcdConnectingStatus(3);
        Serial.println(serviceUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }
    Serial.println(" - Found our service");
    lcdConnectingStatus(4);

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID_rx);
    if (pRemoteCharacteristic == nullptr)
    {
        Serial.print("Failed to find our characteristic UUID: ");
        lcdConnectingStatus(5);
        Serial.println(charUUID_rx.toString().c_str());
        pClient->disconnect();
        return false;
    }
    Serial.println(" - Found our characteristic");
    lcdConnectingStatus(6);
    // Read the value of the characteristic.
    if (pRemoteCharacteristic->canRead())
    {
        String value = pRemoteCharacteristic->readValue(); // geändert wegen Fehlermeldung "conversion from 'String' to non-scalar type 'std::string' {aka 'std::__cxx11::basic_string<char>'} requested"
        Serial.print("The characteristic value was: ");
        Serial.println(value.c_str());
    }

    if (pRemoteCharacteristic->canNotify())
        pRemoteCharacteristic->registerForNotify(notifyCallback);

    BLE_client_connected = true;
    return true;  // geändert wegen Warnmeldung Rückkehr ohne Wert non void
}

void sendCommand(uint8_t *data, uint32_t dataLen)
{
    TRACE;

    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID_tx);

    if (pRemoteCharacteristic)
    {
        pRemoteCharacteristic->writeValue(data, dataLen);
        //commSerial.println("bms request sent");
    }
    else
    {
        Serial.println("Remote TX characteristic not found");
    }
}
