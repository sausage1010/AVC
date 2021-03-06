#include <stdio.h>
#include <time.h>
#include "E101.h"

//Ip address is 10.140.30.203

const int width = 320;
const int MAX_ERROR = 12800;
const bool dev = false;

void findLine(int array[width], int min, int max) {
	
	bool foundLine = false;
	
	while (!foundLine){
		
		take_picture();
		
		for (int i = 0; i < width; i++) {
			array[i] = (int)get_pixel(120, i + 1, 3);
			if (array[i] > max){
				max = array[i];
			}
			else if (array[i] < min) {
				min = array[i];
			}
		}
		
		if (max > 150) {
			foundLine = true;
			set_motor(1, 0);
			set_motor(2, 0);
		}
		else {
			set_motor(1, 50);
			set_motor(2, 50);
		}
	}
}

int openGate(){
	 int i = -1;
	 while(i < 0){
		i = connect_to_server("130.195.6.196", 1024);
		printf("%d", i);
	}
	send_to_server("Please");
	char password[24]; 
	receive_from_server(password);
	printf("%s", password);
	send_to_server(password);
}

int main() {
	
	init();
	sleep1(15, 0);
	openGate();

	FILE* file;
	file = fopen("log.txt", "w");
	
	int error0 = 0;
	int error1 = 0;
	int threshold;
	int vGo = 40; //set this to the speed that you want the robot to travel at by default
	int dv; //difference in speed between the two motors
	int vL, vR;
	int pixRow[320];
	int nwp = 0;
	//change these to adjust
	float Kp = 0.5f; //How aggresively the robot responds to the line not being in the center of the camera
	float Kd = 0.5f;
	
	int diff_e;
	float diff_t;
	float diff;
	
	int max = 0;
	int min = 255;
	
	clock_t t0 = clock(); // set this right before the loop starts so that there is actually a start time
	
	//note that negative means that the line is on the left and that the robot need to move right
	while(true) {
		//set error1 to 0
		error1 = 0;
		nwp = 0;
		max = 0;
		min = 255;
		//take a picture
		take_picture();
				
		//make an array of all the pixels in a row of the image and set max and min
		for (int i = 0; i < width; i++) {
			pixRow[i] = (int)get_pixel(120, i+1, 3);
			if (pixRow[i] > max){
				max = pixRow[i];
			}
			else if (pixRow[i] < min) {
				min = pixRow[i];
			}
		}
		
		if (max < 100) {
			findLine(pixRow, min, max);
		}
				
		//set the threshold to equal half the difference between the max and the minumum
		threshold = (max+min)/2;
		if (dev) {
			fprintf(file, "max = %d\nmin = %d\nthreshold = %d\n", max, min, threshold);
		}
		
		//go through the array again and set white pixels
		for (int i = 0; i < width; i++) {
			if (pixRow[i] > threshold) {
				pixRow[i] = (i-160); //set this back to width/2 when it works
				nwp++;
			}
			else {
				pixRow[i] = 0;
			}
		}
		

		if (dev) {
			fprintf(file, "nwp = %d\n", nwp);
		}
		
		//set the error value
		int j = width - 1;
		for (int i = 0; i < (width/2); i++) {
			error1 = error1 + (pixRow[i] + pixRow[j]);
			j--;
		}

		if (dev) {
			fprintf(file, "error1(1) = %d\n", error1);
		}

			
		//normalise error (error/nwp) to make it so thiccccer lines dont fuck with my baby
		error1 = error1/nwp;
		if (dev) {
			fprintf(file, "error1(2) = %d\n", error1);
		}

		
		//make error a value between 0 and 255 (tbh it's not but I don't even know)
		error1 = (error1*150)/(MAX_ERROR/nwp);
		
		//get the differential
		diff_e = error1 - error0;
		diff_t = clock() - t0;
		diff = (diff_e/diff_t) * 150;
		t0 = clock();
		error0 = error1;

		if(dev){
			fprintf(file, "diff = %f\n", diff);
			fprintf(file, "final error1 = %d\n", error1);
		}
		// set velocity change
		dv = (int)(error1 * Kp + diff * Kd);

		if(dev) {
			fprintf(file, "dv = %d\n", dv);
		}
		
		//set left and right velocities
		vL = vGo + dv;
		vR = vGo - dv;
		
		set_motor(1, -vL);
		set_motor(2, -vR);

	}
	
	set_motor(1,0);
	set_motor(2,0);
	

	fclose(file);
	return 0;
}
