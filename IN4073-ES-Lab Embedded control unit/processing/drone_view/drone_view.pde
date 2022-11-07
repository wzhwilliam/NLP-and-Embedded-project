// Author: Philip Groet

import processing.net.*;

Server s;

float dronePitch;
float droneRoll;
float droneYaw;

void setup() {
  size(640,640,P3D);
  lights();
  
  s = new Server(this, 12345);
}

float yaw_offset = 0;
float yaw_raw;
void draw() {
  Client c = s.available();
  if (c != null) {
    try { // Try catch because sometimes invalid characters are received which are not valid packets
      String input = c.readStringUntil('\n');
                                // Remove 0 character at end
      String parts[] = split(input.substring(0, input.length()-1), ' ');
      int data[] = int(parts);  // Split values into an array
      println("Received: " + data[0] + " " + data[1] + " " + data[2]);
      
      // The result is in radians. ~182 LSB per degree
      dronePitch = (float)data[1] / 10430;
      droneRoll = (float)data[0] / 10430;
      yaw_raw = -(float)data[2]/10430;
      
      droneYaw = yaw_raw + yaw_offset;
      // Simple I controller to compensate for drift
      yaw_offset = yaw_offset - droneYaw*0.5;
      //droneYaw = (float)data[2] / 10430;
    } catch(Exception e) {}
    
  }
  
  background(0);
  int TEXTSIZE = 15;
  int LINESPACING = 5;
  textSize(TEXTSIZE);
  text("Pitch: "+dronePitch/PI*180+"°", 10, 20);
  text("Roll:  "+droneRoll/PI*180+"°",  10, 20+LINESPACING+TEXTSIZE);
  text("Yaw:   "+droneYaw/PI*180+"°",   10, 20+2*LINESPACING+2*TEXTSIZE);
  text("YawDrift:   "+yaw_offset/PI*180+"°",   10, 20+3*LINESPACING+3*TEXTSIZE);
  text("YawRaw:   "+yaw_raw/PI*180+"°",   10, 20+4*LINESPACING+4*TEXTSIZE);
  
  translate(width/2, height/2, 0);
  
  
  
  rotateX(-PI*3/2 + dronePitch);
  rotateY(droneRoll);
  rotateZ(droneYaw);
  //rotateX(-PI*3/2 + (mouseY-height/2)/MOUSE_DIVIDER);
  //rotateY((mouseX-width/2)/MOUSE_DIVIDER);

  noStroke();
  fill(255);
  
  drawDrone();
}
