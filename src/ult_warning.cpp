#include "ros/ros.h"
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string> 
#include <iostream>
#include <std_msgs/Float64.h>
#include <std_msgs/Float32MultiArray.h>
#include <std_msgs/Int32MultiArray.h>
#include <std_msgs/UInt32MultiArray.h>
#include <std_msgs/Int32.h>
#include <pthread.h>
#include <geometry_msgs/TwistStamped.h>
#include <mutex>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "std_msgs/String.h"

using namespace std;

float safe_distance;
ros::Publisher mCmdvelPub_;
uint8_t speed_running = 1;
string playPath_path;

struct Sensor
{
	float Ultrasonic_data[3];    
	float imu_sensor;
	float robot_power;
        
}Sensor_DATA;

// 前左右超声波数据回调函数
void Ultrasonic_Data_Callback(const std_msgs::Float32MultiArray::ConstPtr& data)
{
	Sensor_DATA.Ultrasonic_data[0]  = data->data[0];
	Sensor_DATA.Ultrasonic_data[1]  = data->data[1];
}

// 后超声波数据回调函数
void Back_Ultrasonic_Data(const std_msgs::Float32MultiArray::ConstPtr& data)
{
	Sensor_DATA.robot_power= data->data[0];
	Sensor_DATA.Ultrasonic_data[2]  = data->data[2];
}

// IMU数据回调函数
void IMU_Data_Callback(const std_msgs::Float64::ConstPtr& data)
{
	Sensor_DATA.imu_sensor = data->data; 
}

// void  warning_player(int num)
// {

//      const char *mplayer[] = {       "mplayer //home/hgrobot/arv_ws/src/ult_warning/"," < /dev/null > /dev/null 2>check_erro.log "};
//      const char *voice_tip[]={       "voice/warning.mp3"};
//      char check_tip[200] = {0};

//      strcpy(check_tip,mplayer[0]);
//      strcat(check_tip,voice_tip[num]);
//      strcat(check_tip,mplayer[1]);     
//      system(check_tip);
//      usleep(1000*500);

// }

// 语音播报函数
void  warning_player()
{
	ros::NodeHandle nh("~");    //用于launch文件传递参数
	nh.param("playPath_path", playPath_path, std::string("play ./voice.wav"));    //从launch文件获取参数
	// string path = playPath_path;
	const char *playPath = playPath_path.data();
	system(playPath);
	usleep(1000*1000);
	// sleep(3*1000);	//	延时5S
}

// 根据超声波数据判断是否需要进行语音播报
void read_sensor_data(void)
{
	if(((Sensor_DATA.Ultrasonic_data[0] < safe_distance) && (Sensor_DATA.Ultrasonic_data[0] !=0))|| ((Sensor_DATA.Ultrasonic_data[1] < safe_distance) && (Sensor_DATA.Ultrasonic_data[1] != 0)))
	{
		warning_player();	
	}
}

void  clear_sensor_data(void)
{
	Sensor_DATA.Ultrasonic_data[0] = 0;
	Sensor_DATA.Ultrasonic_data[1] = 0;
	Sensor_DATA.Ultrasonic_data[2] = 0;
    Sensor_DATA.robot_power = 0;
	Sensor_DATA.imu_sensor = 0;
}

// AGV速度控制
void *speed_contorl(void *parameter)
{
	unsigned int count = 0;	
	unsigned int i = 0;	
    geometry_msgs::Twist current_vel; 
	current_vel.linear.x = 0;
	current_vel.linear.y = 0;
	current_vel.linear.z = 0;
	current_vel.angular.x = 0;
	current_vel.angular.y = 0;
	current_vel.angular.z = 0;

	pthread_detach(pthread_self()); 
	while(speed_running)
	{
		if(Sensor_DATA.Ultrasonic_data[0] < safe_distance || Sensor_DATA.Ultrasonic_data[1] < safe_distance || Sensor_DATA.Ultrasonic_data[2] < safe_distance*1000)
		{
			mCmdvelPub_.publish(current_vel);
		}
		count++;
	}
	pthread_exit(NULL);
}

//	主函数
int main(int argc,char **argv)
{
	unsigned int conut = 50; 
	ros::init(argc, argv, "ULT_WARNING_NODE");
	std::string playPath_path;
	ros::NodeHandle n;		//	创建句柄
	ros::NodeHandle nh("~");	
	nh.param<float>("safe_distance", safe_distance, 0.4);	

	ros::Subscriber Ultrasonic_Sub 	= n.subscribe("/Ult_Sensor_Data", 10, &Ultrasonic_Data_Callback);
	ros::Subscriber IMU_Sub 	= n.subscribe("/IMU_Sensor_Data", 10, &IMU_Data_Callback);
	ros::Subscriber back_Ultrasonic_Sub = n.subscribe("/Power_And_Distance", 10,&Back_Ultrasonic_Data);
	nh.param("playPath_path", playPath_path, std::string("play ./voice.wav"));    //从launch文件获取参数

	mCmdvelPub_ = n.advertise<geometry_msgs::Twist>("/cmd_vel", 1, true);	
    // printf("当前安全距离为:%f m r\n",safe_distance);

	pthread_t  speed;
	//	  创建速度控制线程
	pthread_create(&speed, NULL, speed_contorl, NULL);

    while(ros::ok())
	{
		//	根据超声波数据进行语音播报
		read_sensor_data();
        ros::spinOnce();//循环等待回调函数
    }
	speed_running = 0;
  
}



