// David Dang djd122
// 10/3/18
// sine_action_client

#include<ros/ros.h>
#include <actionlib/client/simple_action_client.h>
#include<example_action_server/demoAction.h>
#include <iostream>
#include <string>
using namespace std;

double starttime;
double stoptime;
double diff;

// This function will be called once when the goal completes
// this is optional, but it is a convenient way to get access to the "result" message sent by the server

void doneCb(const actionlib::SimpleClientGoalState& state,
        const example_action_server::demoResultConstPtr& result) {
    stoptime = ros::Time::now().toSec(); // stop stopwatch when receving result
    diff = stoptime-starttime; // calculate elapsed time
    cout<<endl;
    ROS_INFO(" doneCb: server responded with state [%s]", state.toString().c_str());
    ROS_INFO("Time between sending goal and receiving response was: %f seconds", diff);
}

int main(int argc, char** argv) {
    ros::init(argc, argv, "sine_action_client"); // name this node 
    int g_count = 0;
    double g_amp;
    double g_freq;
	double g_cycles;
    // here is a "goal" object compatible with the server, as defined in example_action_server/action
    example_action_server::demoGoal goal;

    // use the name of our server, which is: example_action (named in example_action_server.cpp)
    // the "true" argument says that we want our new client to run as a separate thread (a good idea)
    actionlib::SimpleActionClient<example_action_server::demoAction> action_client("example_action", true);

    // attempt to connect to the server:
    ROS_INFO("waiting for server: ");
    bool server_exists = action_client.waitForServer(ros::Duration(10.0)); // wait for up to 5 seconds
    // something odd in above: does not seem to wait for 5 seconds, but returns rapidly if server not running
    //bool server_exists = action_client.waitForServer(); //wait forever

    if (!server_exists) {
        ROS_WARN("could not connect to server; halting");
        return 0; // bail out; optionally, could print a warning message and retry
    }


    ROS_INFO("connected to action server"); // if here, then we connected to the server;

    while (true) {
        // stuff a goal message:
        //g_count++;
        //goal.input = g_count; // this merely sequentially numbers the goals sent
        //action_client.sendGoal(goal); // simple example--send goal, but do not specify callbacks
        cout<<"Enter an amplitude: ";
        cin>>g_amp;
        cout<<"Enter a frequency (Hz): ";
        cin>>g_freq;
        cout<<"Enter number of cycles: ";
        cin>>g_cycles;
        goal.amp = g_amp;
        goal.freq = g_freq;
        goal.cycles = g_cycles;
        action_client.sendGoal(goal, &doneCb); // we could also name additional callback functions here, if desired
        starttime = ros::Time::now().toSec(); //start stopwatch when sending goal;
    }

    return 0;
}
