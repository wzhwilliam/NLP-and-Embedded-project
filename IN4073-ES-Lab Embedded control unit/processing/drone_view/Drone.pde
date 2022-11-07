int ARM_WIDTH = 50;
int ARM_LENGTH = 200;
int DRONE_HEIGHT = 50;

// Author: Philip Groet
void drawDrone() {
 pushMatrix();
 translate(0, -ARM_LENGTH/2, 0);
 fill(color(255, 0, 0));
 drawArm();
 fill(255);
 popMatrix();
 
 pushMatrix();
 rotateZ(PI/2);
 translate(0, -ARM_LENGTH/2, 0);
 drawArm();
 popMatrix();
 
 pushMatrix();
 rotateZ(PI);
 translate(0, -ARM_LENGTH/2, 0);
 drawArm();
 popMatrix();
 
 pushMatrix();
 rotateZ(-PI/2);
 translate(0, -ARM_LENGTH/2, 0);
 drawArm();
 popMatrix();
 
}

//void drawArm() {
//  rectMode(CORNERS);
//  rect(-ARM_WIDTH/2, ARM_WIDTH/2, ARM_WIDTH/2, -ARM_LENGTH);
//}

void drawArm() {
  box(ARM_WIDTH, ARM_LENGTH, ARM_WIDTH);
}
