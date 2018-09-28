//Sine commmander service

#include <ros/ros.h>
#include <example_ros_service/CommanderServiceMsg.h>
#include <iostream>
#include <string>
using namespace std;

bool Commandercallback(example_ros_service::CommanderServiceMsgRequest& request, example_ros_service::CommanderServiceMsgResponse& response)
{
  ROS_INFO("Callback activated. Generating sinusoidal commmands.");
  double in_amp(request.amp); //convert this to C++-class, so can use member funcs
  double in_freq(request.freq);
  double full_freq = 2*3.14159*in_freq;
  //double x = 0;
  //while(ros::ok()) {
  for(int x=0; x<full_freq; x++) {
    response.sin_cmd = in_amp*sin(x/in_freq);
    //if(x >= full_freq) {
      //x -= full_freq;
    //}
    //x++;
  }
  //}
  return true;
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "commander_service");
  ros::NodeHandle n;

  ros::ServiceServer service = n.advertiseService("sinusoidal_command", Commandercallback);
  ROS_INFO("Ready to give sinusoidal commands.");
  
  ros::spin();

  return 0;
}
