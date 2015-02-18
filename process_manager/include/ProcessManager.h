/*
 * ProcessManager.h
 *
 *  Created on: Nov 1, 2014
 *      Author: Stephan Opfer
 */

#ifndef PROCESSMANAGER_H_
#define PROCESSMANAGER_H_

#define PM_DEBUG // for toggling debug output

#include "ros/ros.h"
#include "process_manager/ProcessCommand.h"
#include "process_manager/ProcessStats.h"
#include "process_manager/ProcessStat.h"
#include <chrono>

using namespace std;

namespace std{
	class thread;
}

namespace supplementary
{

	class SystemConfig;
	class ManagedRobot;
	class ManagedExecutable;
	class RobotMetaData;
	class ExecutableMetaData;
	class ProcessManagerRegistry;

	class ProcessManager
	{
	public:
		ProcessManager(int argc, char** argv);
		virtual ~ProcessManager();
		void start();
		bool isRunning();
		bool selfCheck();
		void initCommunication(int argc, char** argv);

		static void pmSigintHandler(int sig);
		static void pmSigchildHandler(int sig);

		static bool running; /* < has to be static, to be changeable within ProcessManager::pmSignintHandler() */

	private:
		SystemConfig* sc;
		string ownHostname;
		int ownId;
		map<int, ManagedRobot*> robotMap;
		ProcessManagerRegistry* pmRegistry;


		ros::NodeHandle* rosNode;
		ros::AsyncSpinner* spinner;
		ros::Subscriber processCommandSub;
		ros::Publisher processStatePub;


		void handleProcessCommand(process_manager::ProcessCommandPtr pc);
		void changeDesiredProcessStates(process_manager::ProcessCommandPtr pc, bool shouldRun);

		thread* mainThread;
		chrono::microseconds iterationTime;

		void run();
		void searchProcFS();
		void update();
		void report();

	};

} /* namespace alica */

#endif /* PROCESSMANAGER_H_ */
