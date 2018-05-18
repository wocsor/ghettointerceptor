/*
   ghetto pedal by @wocsor. this code makes a lot of assumptions about the gas pedal. you must change these to match yours.
   oh and also i take no responsibility for the things you do with this. use my ugly code at your own risk!
   onions did cry when i showed this hot garbo to them.
   also dbc edits will need to be made. this
*/

//gas_input (acc_cmd) = 0x200
//gas_output = 0x201
//message lenth = 6 bytes

#include <MCP41_Simple.h>
#include <mcp_can.h> //for MCP2515
#include <mcp_can_dfs.h>
#include <SPI.h>

//initialize digipots
MCP41_Simple pot0;
MCP41_Simple pot1;

MCP_CAN CAN(10); //SPI pin for CAN shield

unsigned char accel[] = {0x00, 0x03, 0x63, 0x00, 0x00, 0x00, 0x00, 0x74};
unsigned char val[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
const int CS[3] = {8, 9, 10};
unsigned char len = 0;
unsigned char buf[8];
unsigned int canID;
uint8_t pedal0 = 0;
uint8_t pedal1 = 0;
int accel_cmd;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //init the pots
  pot0.begin(CS[0]);
  pot1.begin(CS[1]);
  //check that can shield works
  //INIT:
  if (CAN_OK == CAN.begin(CAN_500KBPS))     //setting CAN baud rate to 500Kbps
  {
    Serial.println("CAN BUS Shield init ok!");
  }
  else
  {
    Serial.println("CAN BUS Shield init fail");
    Serial.println("Init CAN BUS Shield again");
    //goto INIT;
  }

}

void loop() {
  // put your main code here, to run repeatedly:

  pedal0 = analogRead(A0) / 4; //divide by 4 to get 256 steps from a 10 bit DAC (lower the res)
  pedal1 = analogRead(A1) / 4;
  pot0.setWiper(pedal0); //always send gas pedal position to ECU
  pot1.setWiper(pedal1);

  //read CAN bus for accel_cmd
  CAN.readMsgBuf(&len, buf);    //read data,  len: data length, buf: data buffer
  canID = CAN.getCanId();       //getting the ID of the incoming message

  //filter for the good stuff
  if (canID == 0x200)            //Read msg with ID 0x200, do whatever is below.
  {
    accel_cmd = buf[1];
    accel_cmd = accel_cmd << 8;
    accel_cmd = accel_cmd + buf[2];

    if (((pedal0 + pedal1) / 2)) < 68) { //68 is an example. use the real default values from the pedal + a threshold.

      if (accel_cmd > 0) //accel_cmd > 0 means OP wants the car to accelerate. we set the pots accordingly
      {
        CAN.sendMsgBuf(0x343, 0, len, accel);
        pot0.setWiper((accel_cmd / 32) - 42); //divide by 32 to convert to 8 bits(256 steps), then offset by 42 steps (approx .75v)
        pot1.setWiper(accel_cmd / 32);
      }
      else if (accel_cmd < 0) //accel_cmd < 0, so we forward the Eon's braking messages to the car's CAN bus
      {
        CAN.sendMsgBuf(0x343, 0, len, buf);
        pot0.setWiper(pedal0);
        pot1.setWiper(pedal1);
      }
    }
    else if (((pedal0 + pedal1) / 2) > 68); //gas is being pressed, so we send it right through.
    {
      Serial.print(pedal0);
      Serial.print("\t");
      Serial.print(pedal1);
      Serial.println();
      delay(100);
      val[0] = 0x08;
      val[5] = pedal0;

      CAN.sendMsgBuf(0x201, 0, 6, val); //send whatever is in val
      CAN.sendMsgBuf(0x343, 0, len, accel);
    }
  }
  else { //no CAN signal
    pot0.setWiper(pedal0);
    pot1.setWiper(pedal1);
    }
  }


}
