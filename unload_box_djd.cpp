//David Dang djd122 11/30/18
//unload_box_djd.cpp:
// moves a box under camera, then removes sensed parts from box

//roslaunch cwru_ariac_launch sample_environment.launch fill_demo_shipment:=true
//rosrun kuka_move_as kuka_behavior_as2
//rosrun conveyor_as conveyor_as
//then run this node

//use a RobotBehaviorInterface object to communicate with the robot behavior action server
#include <robot_behavior_interface/RobotBehaviorInterface.h>

//we define a message type for a "part" that includes name, pose and associated location code (e.g. bin # or box location)
#include <inventory_msgs/Part.h>

//a "box inspector" object can compare a packing list to a logical camera image to see how we are doing
#include <box_inspector/box_inspector.h>

//conveyor interface communicates with the conveyor action server
#include <conveyor_as/ConveyorInterface.h>

#include <std_srvs/Trigger.h>
#include <osrf_gear/LogicalCameraImage.h>

const double COMPETITION_TIMEOUT=500.0; // need to  know what this is for the finals;
// want to ship out partial credit before time runs out!

// variables for box camera and quality control sensor data
osrf_gear::LogicalCameraImage g_box_cam1_data;
osrf_gear::LogicalCameraImage g_quality_sensor1_data;

bool g_take_new_snapshot=false;


void model_to_part(osrf_gear::Model model, inventory_msgs::Part &part, unsigned short int location) {
    part.name = model.type;
    part.pose.pose = model.pose;
    part.location = location; //by default
}


void box_cam1CB(const osrf_gear::LogicalCameraImage& message_holder) 
{ 
	if(g_take_new_snapshot) {
		ROS_INFO_STREAM("Image from box camera 1: "<<message_holder<<endl);
		g_box_cam1_data = message_holder;
		g_take_new_snapshot=false;
	}
}


void q_sensor1CB(const osrf_gear::LogicalCameraImage& message_holder) 
{ 
	if(g_take_new_snapshot) {
		ROS_INFO_STREAM("Image from quality sensor 1: "<<message_holder<<endl);
		g_quality_sensor1_data = message_holder;
		g_take_new_snapshot=false;
	}
}


int main(int argc, char** argv) {
    // ROS set-ups:
    ros::init(argc, argv, "box_unloader"); //node name
    ros::NodeHandle nh; // create a node handle; need to pass this to the class constructor
    int ans;
    
    // subscribe to box camera and quality control sensor
    ros::Subscriber box_cam1_subscriber_object= nh.subscribe("/ariac/box_camera_1",1,box_cam1CB);
    ros::Subscriber quality_sensor1_subscriber_object= nh.subscribe("/ariac/quality_control_sensor_1",1,q_sensor1CB);
    
    // Start competition
    ros::ServiceClient startup_client = nh.serviceClient<std_srvs::Trigger>("/ariac/start_competition"); // service name in " "
    std_srvs::Trigger startup_srv;
	startup_client.call(startup_srv);
	startup_srv.response.success=false;	
	while(!startup_srv.response.success) {
		ROS_WARN("Not successful starting competition yet...");
		startup_client.call(startup_srv);
		ros::Duration(0.5).sleep();
	}
	ROS_INFO("Successfully started competition");
    
 
    ROS_INFO("instantiating a RobotBehaviorInterface");
    RobotBehaviorInterface robotBehaviorInterface(&nh); //shipmentFiller owns one as well

    ROS_INFO("instantiating a ConveyorInterface");
    ConveyorInterface conveyorInterface(&nh);
   
    ROS_INFO("instantiating a BoxInspector");
    BoxInspector boxInspector(&nh);

    //instantiate an object of appropriate data type for our move-part commands
    inventory_msgs::Part current_part;

    geometry_msgs::PoseStamped box_pose_wrt_world;  //camera sees box, coordinates are converted to world coords
    
    bool status;    
    int nparts;

    //for box inspector, need to define multiple vectors for args, 
    //box inspector will identify parts and convert their coords to world frame
    //in the present example, desired_models_wrt_world is left empty, so ALL observed parts will be considered "orphaned"
        vector<osrf_gear::Model> desired_models_wrt_world;
        vector<osrf_gear::Model> satisfied_models_wrt_world;
        vector<osrf_gear::Model> misplaced_models_actual_coords_wrt_world;
        vector<osrf_gear::Model> misplaced_models_desired_coords_wrt_world;
        vector<osrf_gear::Model> missing_models_wrt_world;
        vector<osrf_gear::Model> orphan_models_wrt_world;
        vector<int> part_indices_missing;
        vector<int> part_indices_misplaced;
        vector<int> part_indices_precisely_placed;

    
    //use conveyor action  server for multi-tasking
    ROS_INFO("getting a box into position: ");
    int nprint = 0;
    conveyorInterface.move_new_box_to_Q1();  //member function of conveyor interface to move a box to inspection station 1
    while (conveyorInterface.get_box_status() != conveyor_as::conveyorResult::BOX_SEEN_AT_Q1) {
        ros::spinOnce();
        ros::Duration(0.1).sleep();
        nprint++;
        if (nprint % 10 == 0) {
            ROS_INFO("waiting for conveyor to advance a box to Q1...");
        }
    }

    //update box pose,  if possible              
    if (boxInspector.get_box_pose_wrt_world(box_pose_wrt_world)) {
        ROS_INFO_STREAM("box seen at: " << box_pose_wrt_world << endl);
    }
    else {
        ROS_WARN("no box seen.  something is wrong! I quit!!");
        exit(1);
    }
    
    // if survive to here, then box is at Q1 inspection station; 
    
    //inspect the box and classify all observed parts
    boxInspector.update_inspection(desired_models_wrt_world,
        satisfied_models_wrt_world,misplaced_models_actual_coords_wrt_world,
        misplaced_models_desired_coords_wrt_world,missing_models_wrt_world,
        orphan_models_wrt_world,part_indices_missing,part_indices_misplaced,
        part_indices_precisely_placed);
    ROS_INFO("orphaned parts in box: ");
    nparts = orphan_models_wrt_world.size();
    ROS_INFO("num parts seen in box = %d",nparts);
    for (int i=0;i<nparts;i++) {
       ROS_INFO_STREAM("orphaned  parts: "<<orphan_models_wrt_world[i]<<endl);
    }
    
    

    if (boxInspector.get_bad_part_Q1(current_part)) {
        ROS_INFO("found bad part: ");
        ROS_INFO_STREAM(current_part<<endl);
        
        g_take_new_snapshot=true; //check camera viewpoints
        
        ROS_INFO("Identifying name of bad part...");
        
        
        cout<<"enter 1 to attempt to remove bad part: "; //poor-man's breakpoint
        cin>>ans;  
        
        if(ans==1){      
    		//use the robot action server to acquire and dispose of the specified part in the box:
    		status = robotBehaviorInterface.pick_part_from_box(current_part); //remove bad part    		
    	}
    }    

    //after removing the bad part, re-inspect the box:
    boxInspector.update_inspection(desired_models_wrt_world,
        satisfied_models_wrt_world,misplaced_models_actual_coords_wrt_world,
        misplaced_models_desired_coords_wrt_world,missing_models_wrt_world,
        orphan_models_wrt_world,part_indices_missing,part_indices_misplaced,
        part_indices_precisely_placed);
    ROS_INFO("orphaned parts in box: ");
    nparts = orphan_models_wrt_world.size();
    ROS_INFO("num parts seen in box = %d",nparts);
    for (int i=0;i<nparts;i++) {
       ROS_INFO_STREAM("orphaned  parts: "<<orphan_models_wrt_world[i]<<endl);
    }


	//remove the remaining parts
	ROS_INFO("Removing the remaining parts...");
	
    //start w/ first remaining part
    
    //box inspector sees "model", defined in osrf_gear; convert this to our datatype "Part"
    //void model_to_part(osrf_gear::Model model, inventory_msgs::Part &part, unsigned short int location) 
    model_to_part(orphan_models_wrt_world[0], current_part, inventory_msgs::Part::QUALITY_SENSOR_1);

    //use the robot action server to acquire and dispose of the specified part in the box:
    status = robotBehaviorInterface.pick_part_from_box(current_part);

    //SHOULD REPEAT FOR ALL THE PARTS IN THE BOX
    //ALSO, WATCH OUT FOR NO PARTS IN THE BOX--ABOVE WILL CRASH
 
    
    //go to second remaining part
    
    //box inspector sees "model", defined in osrf_gear; convert this to our datatype "Part"
    //void model_to_part(osrf_gear::Model model, inventory_msgs::Part &part, unsigned short int location) 
    model_to_part(orphan_models_wrt_world[1], current_part, inventory_msgs::Part::QUALITY_SENSOR_1);

    //use the robot action server to acquire and dispose of the specified part in the box:
    status = robotBehaviorInterface.pick_part_from_box(current_part);
    
    if(orphan_models_wrt_world.size()==0){
    	ROS_INFO("All remaining parts have been removed");
    	return 0;
    }
}
