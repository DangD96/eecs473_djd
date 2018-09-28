//Sine commmander service

#include <ros/ros.h>
#include <example_ros_service/CommanderServiceMsg.h>
#include <iostream>
#include <string>
using namespace std;

double in_amp;
double in_freq;
#include<std_msgs/Float64.h> 
std_msgs::Float64 g_velocity;
std_msgs::Float64 g_vel_cmd;
std_msgs::Float64 g_force;
std_msgs::Float64 g_sin_cmd;

bool Commandercallback(example_ros_service::CommanderServiceMsgRequest& request, example_ros_service::CommanderServiceMsgResponse& response)
{
  ROS_INFO("Callback activated. Generating sinusoidal commmands.");
  in_amp = request.amp; 
  in_freq = request.freq;
      //double amp;
    //double freq;
    //publish a sinusoidal velocity command on sin_cmd topic; 
    ros::NodeHandle nh; // node handle
    ros::Publisher my_publisher_object = nh.advertise<std_msgs::Float64>("sin_cmd", 1);
    double full_freq = 2*3.14159*in_freq;
    double x = 0; // variable for cycling through frequency
    double dt_commander = 0.02; // 50Hz sample rate
    double sample_rate = 1.0 / dt_commander; // compute update frequency
    while(ros::ok()) {  // cycle through frequency and continually publish sinusoidal command
      g_sin_cmd.data = in_amp*sin(x/in_freq);
      if(x >= full_freq) {
        x -= full_freq; // x = x - full_period --> reset x to zero
      }
      x++; // increment x --> oscillations
      my_publisher_object.publish(g_sin_cmd); // publish sinusoidal command
      ros::Rate naptime(sample_rate);
      ROS_INFO("sinusoidal command = %f", g_sin_cmd.data);
      ros::spinOnce(); //allow data update from callback; 
      naptime.sleep(); // wait for remainder of specified period;
    } 
}

int main(int argc, char **argv) {
    ros::init(argc, argv, "commander_service"); //name this node
    ros::NodeHandle n;

  ros::ServiceServer service = n.advertiseService("sinusoidal_command", Commandercallback);
  ROS_INFO("Ready.");
  ros::spin();

  return 0;

    
    //return 0;
}
