// David Dang djd122
// 10/3/18
// sine_action_server

#include<ros/ros.h>
#include <actionlib/server/simple_action_server.h>
#include<example_action_server/demoAction.h>
#include<std_msgs/Float64.h> 
std_msgs::Float64 g_vel_cmd;
#include <iostream>
#include <string>
using namespace std;

int g_count = 0;
bool g_count_failure = false;
double g_amp;
double g_freq;
double g_cycles;
class ExampleActionServer {
private:

    ros::NodeHandle nh_;  // we'll need a node handle; get one upon instantiation

    // this class will own a "SimpleActionServer" called "as_".
    // it will communicate using messages defined in example_action_server/action/demo.action
    // the type "demoAction" is auto-generated from our name "demo" and generic name "Action"
    actionlib::SimpleActionServer<example_action_server::demoAction> as_;
    
    // here are some message types to communicate with our client(s)
    example_action_server::demoGoal goal_; // goal message, received from client
    example_action_server::demoResult result_; // put results here, to be sent back to the client when done w/ goal
    example_action_server::demoFeedback feedback_; 
    // would need to use: as_.publishFeedback(feedback_); to send incremental feedback to the client

public:
    ExampleActionServer(); //define the body of the constructor outside of class definition

    ~ExampleActionServer(void) {
    }
    // Action Interface
    void executeCB(const actionlib::SimpleActionServer<example_action_server::demoAction>::GoalConstPtr& goal);
};

//implementation of the constructor:
ExampleActionServer::ExampleActionServer() :
   as_(nh_, "example_action", boost::bind(&ExampleActionServer::executeCB, this, _1),false) 
// in the above initialization, we name the server "example_action"
//  clients will need to refer to this name to connect with this server
{
    ROS_INFO("in constructor of exampleActionServer...");
    // do any other desired initializations here...specific to your implementation

    as_.start(); //start the server running
}

void ExampleActionServer::executeCB(const actionlib::SimpleActionServer<example_action_server::demoAction>::GoalConstPtr& goal) {
    
    ros::NodeHandle nh;
    ros::Publisher my_publisher_object = nh.advertise<std_msgs::Float64>("vel_cmd", 1);    
    double full_freq = 2*3.14159*goal->freq;
    double x = 0; // variable for cycling through frequency
    double dt_commander = 0.02; // 50Hz sample rate
    double sample_rate = 1.0 / dt_commander; // compute update frequency 
    int cycle_count = 0; // keep track of completed cycles   
    while(ros::ok()) {      
      g_vel_cmd.data = goal->amp*sin(x/goal->freq);
      if(x >= full_freq) {
        x -= full_freq; // x = x - full_period --> reset x to zero
        cycle_count++;
      }
      x++; // increment x --> oscillations
      my_publisher_object.publish(g_vel_cmd); // publish sinusoidal command
      ros::Rate naptime(sample_rate);
      result_.output = g_vel_cmd.data;
      
      //ROS_INFO("Sinusoidal command = %f", g_vel_cmd.data);
      ros::spinOnce(); //allow data update from callback; 
      naptime.sleep(); // wait for remainder of specified period; ;
	  if (cycle_count == goal->cycles) {
	    as_.setSucceeded(result_);
	    break;
	  }
    }
    ROS_INFO("Specified cycles completed");
}

int main(int argc, char** argv) {
    ros::init(argc, argv, "sine_action_server"); // name this node 

    ROS_INFO("instantiating the demo action server: ");

    ExampleActionServer as_object; // create an instance of the class "ExampleActionServer"
    
    ROS_INFO("going into spin");
    // from here, all the work is done in the action server, with the interesting stuff done within "executeCB()"
    // you will see 5 new topics under example_action: cancel, feedback, goal, result, status
    while (!g_count_failure && ros::ok()) {
        ros::spinOnce(); //normally, can simply do: ros::spin();  
        // for debug, induce a halt if we ever get our client/server communications out of sync
    }
    return 0;
}

