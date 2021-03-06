// David Dang djd122 12/7/18

// jenga_tower_builder: 
// illustrates use of a generic action client that communicates with
// an action server called "cartMoveActionServer"
// the actual action server can be customized for a specific robot, whereas
// this client is robot agnostic

//launch with roslaunch irb140_description irb140.launch, which places a block at x=0.5, y=0

// then run this node

#include <ros/ros.h>
#include <actionlib/client/simple_action_client.h>
#include <actionlib/client/terminal_state.h>
#include <arm_motion_action/arm_interfaceAction.h>
#include <cartesian_motion_commander/cart_motion_commander.h>
#include <Eigen/Eigen>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <xform_utils/xform_utils.h>
#include <std_srvs/SetBool.h>
//#include <irb120_fk_ik/irb120_kinematics.h>  //access to forward and inverse kinematics
//#include <fk_ik_virtual/fk_ik_virtual.h> //defines the base class with virtual fncs
// this is useful to keep the motion planner generic
//#include "robot_specific_fk_ik_mappings.h" //these two files are needed to provide robot-specific info to generic planner
//#include "robot_specific_names.h"
//#include <generic_cartesian_planner/generic_cartesian_planner.h>
//#include <cartesian_interpolator/cartesian_interpolator.h>
#include <geometry_msgs/PoseStamped.h>
#include <trajectory_msgs/JointTrajectory.h>
#include<sensor_msgs/JointState.h>
//add these to use the "magic" object finder action server
#include <actionlib/client/simple_action_client.h>
#include <actionlib/client/terminal_state.h>
#include <magic_object_finder/magicObjectFinderAction.h>
using namespace std;

geometry_msgs::PoseStamped g_block_data;


/*void blockCB(const geometry_msgs::PoseStamped& message_holder) 
{ 
	g_block_data = message_holder;
	ROS_INFO("Received block pose from OpenCV node");	
}*/


//some magic numbers--place at the top of the program
//std::vector<double> g_planner_joint_weights{3, 3, 2, 1, 1, 0.5}; //specify weights to use for planner optimization

//Hard-coded name of object of interest
string g_object_name("toy_block");  //hard-coded object
string g_object_name2("toy_block2");
string g_object_name3("toy_block3");
int g_found_object_code; //global to communicate between callback and main: true if named object was found
geometry_msgs::PoseStamped g_perceived_object_pose; //global to communicate between callback and main: pose  of found object

ros::Publisher *g_pose_publisher; //make this global so callback can access it--for displaying object frames in rviz


//this callback function receives a result from the magic object finder action server
//it sets g_found_object_code to true or false, depending on whether the  object was found
//if the object was found, then components of g_perceived_object_pose are filled in
void objectFinderDoneCb(const actionlib::SimpleClientGoalState& state,
        const magic_object_finder::magicObjectFinderResultConstPtr& result) {
    ROS_INFO(" objectFinderDoneCb: server responded with state [%s]", state.toString().c_str());
    g_found_object_code=result->found_object_code;
    ROS_INFO("got object code response = %d; ",g_found_object_code);
    if (g_found_object_code==magic_object_finder::magicObjectFinderResult::OBJECT_NOT_FOUND) {
        ROS_WARN("object-finder responded: object not found");
    }
    else if (g_found_object_code==magic_object_finder::magicObjectFinderResult::OBJECT_FOUND) {
        ROS_INFO("found object!");
         g_perceived_object_pose= result->object_pose;
         ROS_INFO("got pose x,y,z = %f, %f, %f",g_perceived_object_pose.pose.position.x,
                 g_perceived_object_pose.pose.position.y,
                 g_perceived_object_pose.pose.position.z);

         ROS_INFO("got quaternion x,y,z, w = %f, %f, %f, %f",g_perceived_object_pose.pose.orientation.x,
                 g_perceived_object_pose.pose.orientation.y,
                 g_perceived_object_pose.pose.orientation.z,
                 g_perceived_object_pose.pose.orientation.w);
         g_pose_publisher->publish(g_perceived_object_pose);  //this is to enable display of pose of found object in rviz
    }
    else {
        ROS_WARN("object not found!");
    }
}


int main(int argc, char** argv) {
    ros::init(argc, argv, "jenga_tower_builder"); // name this node 
    ros::NodeHandle nh; //standard ros node handle     
    CartMotionCommander cart_motion_commander; 
    XformUtils xformUtils; 
    ros::ServiceClient client = nh.serviceClient<std_srvs::SetBool>("/sticky_finger/link6");
    std_srvs::SetBool srv;
    srv.request.data = true;
    
    actionlib::SimpleActionClient<magic_object_finder::magicObjectFinderAction> object_finder_ac("object_finder_action_service", true);
    bool finished_before_timeout=false; 
    // attempt to connect to the server:
    ROS_INFO("waiting for server: ");
    bool server_exists = false;
    while ((!server_exists)&&(ros::ok())) {
        server_exists = object_finder_ac.waitForServer(ros::Duration(0.5)); // 
        ros::spinOnce();
        ros::Duration(0.5).sleep();
        ROS_INFO("retrying...");
    }
    ROS_INFO("connected to object_finder action server"); // if here, then we connected to the server; 
    ros::Publisher pose_publisher = nh.advertise<geometry_msgs::PoseStamped>("triad_display_pose", 1, true); 
    g_pose_publisher = &pose_publisher;
    magic_object_finder::magicObjectFinderGoal object_finder_goal; //instantiate goal message to communicate with magic_object_finder
    
    
		//the following makes an inquiry for the pose of the part of interest
    	//specify the part name, send it in the goal message, wait for and interpret the result

		//"go look for gear_part"
    	object_finder_goal.object_name = g_object_name.c_str(); //convert string object to old     C-style string data
    	object_finder_ac.sendGoal(object_finder_goal,&objectFinderDoneCb); //request object 	finding via action server
		finished_before_timeout = object_finder_ac.waitForResult(ros::Duration(10.0)); //wait for a max time for response
    //NOTE: could do something else here (if useful) while waiting for response from action server
    	if (!finished_before_timeout) {
            ROS_WARN("giving up waiting on result "); //this should not happen; should get result of found or not-found
            return 1;
        }
    //check the result code to see if object was found or not
    	if (g_found_object_code == magic_object_finder::magicObjectFinderResult::OBJECT_FOUND) {
        	ROS_INFO("found object!");
    	}    
    	else {
        	ROS_WARN("object not found!  Quitting");
        	return 1;
    	}

//if no warnings, then object was found
//done with inquiry.  If here, then part pose is in g_perceived_object_pose.  Use it to compute robot motion 

	//subscribe to block_pose topic 
	//ros::Subscriber block_pose_subscriber= nh.subscribe("block_pose",1,blockCB);

    Eigen::VectorXd joint_angles;
    Eigen::Vector3d dp_displacement;
    int rtn_val;
    int njnts;
    int nsteps;
    double arrival_time;
    geometry_msgs::PoseStamped tool_pose, tool_pose_home;

    bool traj_is_valid = false;
    int rtn_code;

    nsteps = 10;
    arrival_time = 2.0;



    Eigen::Vector3d b_des, n_des, t_des, O_des;
    Eigen::Matrix3d R_gripper;
    b_des << 0, 0, -1;
    n_des << -1, 0, 0;
    t_des = b_des.cross(n_des);

    R_gripper.col(0) = n_des;
    R_gripper.col(1) = t_des;
    R_gripper.col(2) = b_des;

    //- Translation: [0.450, -0.000, 0.367]

    O_des << 0.5, 0.3, 0.3;
    Eigen::Affine3d tool_affine;
    tool_affine.linear() = R_gripper;
    tool_affine.translation() = O_des; 

    tool_pose = xformUtils.transformEigenAffine3dToPoseStamped(tool_affine, "system_ref_frame");
    ROS_INFO("requesting plan to gripper-down pose:");
    xformUtils.printPose(tool_pose);
    rtn_val = cart_motion_commander.plan_jspace_traj_current_to_tool_pose(nsteps, arrival_time, tool_pose);
    if (rtn_val == arm_motion_action::arm_interfaceResult::SUCCESS) {
        ROS_INFO("successful plan; command execution of trajectory");
        rtn_val = cart_motion_commander.execute_planned_traj();
        ros::Duration(arrival_time + 0.2).sleep();
    } else {
        ROS_WARN("unsuccessful plan; rtn_code = %d", rtn_val);
    }

    while (ros::ok()) {
        //move out of way for camera view:
        ROS_INFO("moving out of camera view");
        tool_pose.pose.position.y=0.3; 
        tool_pose.pose.position.z = 0.3; //0.01;          
        ROS_INFO("requesting plan to descend:");
        xformUtils.printPose(tool_pose);
        rtn_val = cart_motion_commander.plan_cartesian_traj_qprev_to_des_tool_pose(nsteps, arrival_time, tool_pose);
        if (rtn_val == arm_motion_action::arm_interfaceResult::SUCCESS) {
            ROS_INFO("successful plan; command execution of trajectory");
            rtn_val = cart_motion_commander.execute_planned_traj();
            ros::Duration(arrival_time + 0.2).sleep();
        } else {
            ROS_WARN("unsuccessful plan; rtn_code = %d", rtn_val);
        }        
        
        //move to approach pose:
        ROS_INFO("moving to approach pose");
        //tool_pose.pose.position.y = g_block_data.pose.position.y;
        tool_pose.pose.position.x = 0.6;
        tool_pose.pose.position.y = 0;
        //tool_pose.pose.position.y=0.0; 
        tool_pose.pose.position.z = 0.05; //0.01;          
        ROS_INFO("requesting plan to descend:");
        xformUtils.printPose(tool_pose);
        rtn_val = cart_motion_commander.plan_cartesian_traj_qprev_to_des_tool_pose(nsteps, arrival_time, tool_pose);
        if (rtn_val == arm_motion_action::arm_interfaceResult::SUCCESS) {
            ROS_INFO("successful plan; command execution of trajectory");
            rtn_val = cart_motion_commander.execute_planned_traj();
            ros::Duration(arrival_time + 0.2).sleep();
        } else {
            ROS_WARN("unsuccessful plan; rtn_code = %d", rtn_val);
        }           
        
        
        ROS_INFO("moving to grasp pose");
        //lower tool to approach part to grasp
        //tool_pose.pose.position.y=0; 
        tool_pose.pose.position.z = 0.0340; //block is 0.035 high      

        ROS_INFO("requesting plan to descend:");
        xformUtils.printPose(tool_pose);
        rtn_val = cart_motion_commander.plan_cartesian_traj_qprev_to_des_tool_pose(nsteps, arrival_time, tool_pose);
        if (rtn_val == arm_motion_action::arm_interfaceResult::SUCCESS) {
            ROS_INFO("successful plan; command execution of trajectory");
            rtn_val = cart_motion_commander.execute_planned_traj();
            ros::Duration(arrival_time + 0.2).sleep();
        } else {
            ROS_WARN("unsuccessful plan; rtn_code = %d", rtn_val);
        }

        ROS_INFO("enabling vacuum gripper");
        //enable the vacuum gripper:
        srv.request.data = true;
        while (!client.call(srv) && ros::ok()) {
            ROS_INFO("Sending command to gripper...");
            ros::spinOnce();
            ros::Duration(0.5).sleep();
        }


		//depart after enabling vacuum gripper
        ROS_INFO("requesting plan to depart with grasped object:");
        tool_pose.pose.position.z = 0.3;         

        xformUtils.printPose(tool_pose);
        rtn_val = cart_motion_commander.plan_cartesian_traj_qprev_to_des_tool_pose(nsteps, arrival_time, tool_pose);
        if (rtn_val == arm_motion_action::arm_interfaceResult::SUCCESS) {
            ROS_INFO("successful plan; command execution of trajectory");
            rtn_val = cart_motion_commander.execute_planned_traj();
            ros::Duration(arrival_time + 0.2).sleep();
        } else {
            ROS_WARN("unsuccessful plan; rtn_code = %d", rtn_val);
        }



		//move block to my specified location		
		ROS_INFO("Moving to other location");
        tool_pose.pose.position.y=0.5; 
        //tool_pose.pose.position.z = 0.0340; //block is 0.035 high      

        ROS_INFO("requesting plan to descend:");
        xformUtils.printPose(tool_pose);
        rtn_val = cart_motion_commander.plan_cartesian_traj_qprev_to_des_tool_pose(nsteps, arrival_time, tool_pose);
        if (rtn_val == arm_motion_action::arm_interfaceResult::SUCCESS) {
            ROS_INFO("successful plan; command execution of trajectory");
            rtn_val = cart_motion_commander.execute_planned_traj();
            ros::Duration(arrival_time + 0.2).sleep();
        } else {
            ROS_WARN("unsuccessful plan; rtn_code = %d", rtn_val);
        }




        //disable the vacuum gripper:
        srv.request.data = false;
        while (!client.call(srv) && ros::ok()) {
            ROS_INFO("Sending command to gripper...");
            ros::spinOnce();
            ros::Duration(0.5).sleep();
        }
    }

    return 0;
}

