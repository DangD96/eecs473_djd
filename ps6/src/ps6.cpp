//David Dang djd122
// 11/6/18
// ps6.cpp

// roslaunch osrf_gear sample_environment.launch
// rosservice call /ariac/start_competition
// rosservice call /ariac/conveyor/control "power: 100"
// rosservice call /ariac/drone "shipment_type: order_0_shipment_0"

#include <ros/ros.h>
#include <std_srvs/Trigger.h> 
#include <osrf_gear/ConveyorBeltControl.h> 
#include <osrf_gear/DroneControl.h> 
#include <osrf_gear/LogicalCameraImage.h> 
#include <iostream>
#include <string>
using namespace std;

bool g_take_new_snapshot=false;
osrf_gear::LogicalCameraImage g_cam2_data;

void cam2CB(const osrf_gear::LogicalCameraImage& message_holder) 
{ 
	if(g_take_new_snapshot) {
		ROS_INFO_STREAM("Image from cam2: "<<message_holder<<endl);
		g_cam2_data = message_holder;
		g_take_new_snapshot=false;
	}
}


int main(int argc, char **argv) {
    ros::init(argc, argv, "ps6");
    ros::NodeHandle n;
    
    ros::Subscriber cam2_subscriber_object= n.subscribe("/ariac/logical_camera_2",1,cam2CB); 
    
    // Start sytem
    ros::ServiceClient startup_client = n.serviceClient<std_srvs::Trigger>("/ariac/start_competition"); // service name in " "
    std_srvs::Trigger startup_srv;
	startup_client.call(startup_srv);
	startup_srv.response.success=false;	
	while(!startup_srv.response.success) {
		ROS_WARN("Not successful starting up yet...");
		startup_client.call(startup_srv);
		ros::Duration(0.5).sleep();
	}
	ROS_INFO("Got success response from startup client");

	
	// Start conveyor belt
	ros::ServiceClient conveyor_client = n.serviceClient<osrf_gear::ConveyorBeltControl>("/ariac/conveyor/control");
    osrf_gear::ConveyorBeltControl conveyor_srv;    
    conveyor_srv.request.power = 100.0;
    conveyor_srv.response.success=false;
	while(!conveyor_srv.response.success) {
		ROS_WARN("Not successful starting conveyor yet...");
		conveyor_client.call(conveyor_srv);
		ros::Duration(0.5).sleep();
	}
	ROS_INFO("Got success response from conveyor service");
	
	

	// Monitor logical camera 2 for shipping box
	// Halt conveyor when logical camera 2 reports that z-value of shipping box is close to zero	
	ROS_INFO("Waiting for box");
	while(g_cam2_data.models.size()<1) { //as long as camera sees no objects, keep waiting until it sees something		
		g_take_new_snapshot=true;
		ros::spinOnce();
		ros::Duration(0.5).sleep();		
		if(g_cam2_data.models.size()==1) {break;}
	}
	ROS_INFO("I see a box");
	
	while(ros::ok()) {
		g_take_new_snapshot=true;
		double z_value = g_cam2_data.models[0].pose.position.z; 
		
		if(z_value > 0 - 0.05 || z_value < 0 + 0.05) {
		
			// Stop belt and wait for 5 seconds	
			conveyor_srv.request.power = 0;
			conveyor_srv.response.success=false;
			while(!conveyor_srv.response.success) {
				ROS_WARN("Not successful stopping conveyor yet...");
				conveyor_client.call(conveyor_srv);
				ros::Duration(0.5).sleep();
			}
			ROS_INFO("Successfully stopped conveyor belt");
			ROS_INFO("Waiting for 5 seconds...");
			ros::Duration(5).sleep();
			cout<<"z-value of box when conveyor halted was: "<<z_value<<endl;
		
			// Turn conveyor belt back on	
			conveyor_srv.request.power = 100.0;
			conveyor_srv.response.success=false;
			while(!conveyor_srv.response.success) {
				ROS_WARN("Not successful restarting conveyor yet...");
				conveyor_client.call(conveyor_srv);
				ros::Duration(0.5).sleep();
			}
			ROS_INFO("Successfully restarted conveyor");
		}		
		  
		break; // break out of while loop once belt has been stopped for 5 seconds and restarted		
	}
	

	// Tell drone to pick up box
	ros::ServiceClient drone_client = n.serviceClient<osrf_gear::DroneControl>("/ariac/drone");
    osrf_gear::DroneControl drone_srv;
    drone_srv.request.shipment_type = "dummy";
    drone_srv.response.success=false; 
	while(!drone_srv.response.success) {
		//ROS_WARN("Not successful calling drone yet...");
		drone_client.call(drone_srv);
		ros::Duration(0.5).sleep();
	}
	ROS_INFO("Got success response from drone service");	
	
	
	// Stop conveyor belt
	conveyor_srv.request.power = 0;
	conveyor_srv.response.success=false;
	while(!conveyor_srv.response.success) {
		ROS_WARN("Not successful stopping conveyor yet...");
		conveyor_client.call(conveyor_srv);
		ros::Duration(0.5).sleep();
	}
	ROS_INFO("Successfully stopped conveyor belt - Finished");
	
    return 0;
}
