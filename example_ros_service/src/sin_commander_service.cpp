// David Dang djd122
// 10/1/18
// sin_commander node modified to be a service

#include<ros/ros.h> 
#include<std_msgs/Float64.h> 
std_msgs::Float64 g_velocity;
std_msgs::Float64 g_vel_cmd;
std_msgs::Float64 g_force;
#include <example_ros_service/CommanderServiceMsg.h>
#include <iostream>
#include <string>
using namespace std;

bool Commandercallback(example_ros_service::CommanderServiceMsgRequest& request, example_ros_service::CommanderServiceMsgResponse& response)
{
    ROS_INFO("Callback activated.");  
    double in_amp(request.amp); 
    double in_freq(request.freq); 
    response.received = true;
    ros::NodeHandle nh;
    ros::Publisher my_publisher_object = nh.advertise<std_msgs::Float64>("vel_cmd", 1);    
    double full_freq = 2*3.14159*in_freq;
    double x = 0; // variable for cycling through frequency
    double dt_commander = 0.02; // 50Hz sample rate
    double sample_rate = 1.0 / dt_commander; // compute update frequency    
    while(ros::ok()) {  // cycle through frequency and continually publish sinusoidal command    
      g_vel_cmd.data = in_amp*sin(x/in_freq);
      if(x >= full_freq) {
        x -= full_freq; // x = x - full_period --> reset x to zero
      }
      x++; // increment x --> oscillations
      my_publisher_object.publish(g_vel_cmd); // publish sinusoidal command
      ros::Rate naptime(sample_rate);
      ROS_INFO("Sinusoidal command = %f", g_vel_cmd.data);
      ros::spinOnce(); //allow data update from callback; 
      naptime.sleep(); // wait for remainder of specified period; 
    }
    return 0;
}

int main(int argc, char **argv) {
    ros::init(argc, argv, "sin_commander_service"); //name this node
    ros::NodeHandle n; // node handle
    ros::ServiceServer service = n.advertiseService("sinusoidal_command", Commandercallback);
    ROS_INFO("Ready to generate sinusoidal commands.");    
    ros::spin();
    return 0;
}
