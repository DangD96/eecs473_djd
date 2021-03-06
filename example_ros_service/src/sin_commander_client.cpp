// David Dang djd122
// 10/1/18

#include <ros/ros.h>
#include <example_ros_service/CommanderServiceMsg.h> 
#include <iostream>
#include <string>
using namespace std;

int main(int argc, char **argv) {
    ros::init(argc, argv, "sin_commander_client");
    ros::NodeHandle n;
    ros::ServiceClient client = n.serviceClient<example_ros_service::CommanderServiceMsg>("sinusoidal_command");
    example_ros_service::CommanderServiceMsg srv;
    bool received; 
    double in_amp;
    double in_freq; 
    while(ros::ok()) { 
      cout<<"Enter an amplitude: ";
      cin>>in_amp;
      cout<<"Enter a frequency (Hz): ";
      cin>>in_freq;
      srv.request.amp = in_amp; 
      srv.request.freq = in_freq;       
      if (client.call(srv)) {
        if(srv.response.received) {
	  continue;         
        }        
      }
    }
    return 0;
}
