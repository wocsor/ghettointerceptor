/*
   ghetto pedal by @wocsor. this code makes a lot of assumptions about the gas pedal. you must change these to match yours.
   oh and also i take no responsibility for the things you do with this. use my ugly code at your own risk!
   onions did cry when i showed this hot garbo to them.
   also dbc edits will need to be made. this
*/

#include <MCP41_Simple.h>
#include <mcp_can.h> //for MCP2515
#include <mcp_can_dfs.h>
#include <SPI.h>

//initialize digipots
MCP41_Simple pot0;
MCP41_Simple pot1;

MCP_CAN CAN(10); //SPI pin for CAN shield

const int CS[3] = {8, 9, 10};
unsigned char len = 0;
unsigned char buf[8];
unsigned int canID;
uint8_t pedal0 = 0;
uint8_t pedal1 = 0;
unsigned int steer_request;
int steer_torque_cmd;
unsigned int scale;

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
  pot0.setWiper(128);
  pot1.setWiper(128);

}

void loop() {
  // put your main code here, to run repeatedly:

  pedal0 = analogRead(A0) / 4; //divide by 4 to get 256 steps from a 10 bit ADC (lower the res)
  pedal1 = analogRead(A1) / 4;

  //read CAN bus for accel_cmd
  CAN.readMsgBuf(&len, buf);    //read data,  len: data length, buf: data buffer
  canID = CAN.getCanId();       //getting the ID of the incoming message

  //filter for the good stuff
  if (canID == 0x2e4)            //Read msg with ID 0x2e4 (LKAS), do whatever is below.
  {
    steer_torque_cmd = buf[1];
    steer_torque_cmd = steer_torque_cmd << 8;
    steer_torque_cmd = steer_torque_cmd + buf[2];
    scale = abs(steer_torque_cmd) / 32;

    steer_request = buf[0] & 0xf >> 1;

    if (steer_request == 1) { //steer is being requested by EON so we send our spoofed torques.

      if (steer_torque_cmd > 0)
      {
        pot0.setWiper(128 + scale); //divide by 32 to convert to 8 bits(256 steps), then offset by 42 steps (approx .75v)
        pot1.setWiper(128 - scale);
      }
      else if (steer_torque_cmd < 0) //accel_cmd < 0, so we forward the Eon's braking messages to the car's CAN bus
      {
        pot0.setWiper(128 - scale);
        pot1.setWiper(128 + scale);
      }
    }
    else if (steer_request == 0); //steer not being requested. pass torque values along
    {
      pot0.setWiper(pedal0);
      pot1.setWiper(pedal1);
    }
  }
  else { //no CAN signal
    pot0.setWiper(pedal0);
    pot1.setWiper(pedal1);
    Serial.println("no can!");
  }
}
