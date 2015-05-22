/*
 * ControlledRobot.cpp
 *
 *  Created on: Feb 27, 2015
 *      Author: Stephan Opfer
 */

#include <rqt_pm_control/ControlledRobot.h>
#include <rqt_pm_control/ControlledExecutable.h>
#include <RobotExecutableRegistry.h>
#include <ui_RobotProcessesWidget.h>
#include <limits.h>
#include <process_manager/ProcessCommand.h>
#include <ros/ros.h>
#include "rqt_pm_control/ControlledProcessManager.h"
#include "rqt_pm_control/PMControl.h"
#include "ui_ProcessWidget.h"
#include "ExecutableMetaData.h"

namespace rqt_pm_control
{
	ControlledRobot::ControlledRobot(string robotName, int robotId, ControlledProcessManager* parentProcessManager) :
			RobotMetaData(robotName, robotId), parentProcessManager(parentProcessManager), robotProcessesQFrame(new QFrame()), _robotProcessesWidget(
					new Ui::RobotProcessesWidget())
	{
		// setup gui stuff
		this->_robotProcessesWidget->setupUi(this->robotProcessesQFrame);
		this->_robotProcessesWidget->robotHostLabel->setText(QString(string(this->name + " on " + this->parentProcessManager->name).c_str()));

		QObject::connect(this->_robotProcessesWidget->bundleComboBox, SIGNAL(activated(QString)), this, SLOT(updateBundles(QString)));

		// enter bundles in combo box
		for (auto bundleEntry : this->parentProcessManager->parentPMControl->bundlesMap)
		{
			this->_robotProcessesWidget->bundleComboBox->insertItem(INT_MAX, QString(bundleEntry.first.c_str()), QVariant(bundleEntry.first.c_str()));
		}

		// construct all known executables
		const vector<supplementary::ExecutableMetaData*>& execMetaDatas = this->parentProcessManager->parentPMControl->pmRegistry->getExecutables();
		ControlledExecutable* controlledExec;
		for (auto execMetaDataEntry : execMetaDatas)
		{
			controlledExec = new ControlledExecutable(execMetaDataEntry, this);
			this->controlledExecMap.emplace(execMetaDataEntry->id, controlledExec);
		}

		this->parentProcessManager->addRobot(this->robotProcessesQFrame);
	}

	ControlledRobot::~ControlledRobot()
	{
		cout << "CR: 1" <<endl;
		for (auto execEntry : this->controlledExecMap)
		{
			delete execEntry.second;
		}
		//this->parentProcessManager->removeRobot(robotProcessesQFrame);
		cout << "CR: 2" <<endl;
		//delete _robotProcessesWidget;
		cout << "CR: 3" <<endl;
		delete robotProcessesQFrame;
		cout << "CR: 4" <<endl;
	}

	void ControlledRobot::handleProcessStat(chrono::system_clock::time_point timeMsgReceived, process_manager::ProcessStat ps)
	{
		this->timeLastMsgReceived = timeMsgReceived;
		auto controlledExecEntry = this->controlledExecMap.find(ps.processKey);
		if (controlledExecEntry != this->controlledExecMap.end())
		{ // executable is already known

			// update the statistics of the ControlledExecutable
			controlledExecEntry->second->handleStat(timeMsgReceived, ps);
		}
		else
		{ // executable is unknown
			cerr << "ControlledRobot: Received processStat for unknown executable with process key " << ps.processKey << endl;
			return;
		}
	}

	void ControlledRobot::updateGUI(chrono::system_clock::time_point now)
	{
		for (auto controlledExecEntry : this->controlledExecMap)
		{
			controlledExecEntry.second->updateGUI(now);
		}
	}

	void ControlledRobot::updateBundles(QString text)
	{
		if (text == "ALL")
		{
			for (auto controlledExecMapEntry : this->controlledExecMap)
			{
				controlledExecMapEntry.second->processWidget->show();
				if (controlledExecMapEntry.second->metaExec->name != "roscore")
				{
					controlledExecMapEntry.second->_processWidget->checkBox->setEnabled(true);
				}
			}
			return;
		}

		if (text == "RUNNING")
		{
			for (auto controlledExecMapEntry : this->controlledExecMap)
			{
				switch (controlledExecMapEntry.second->state)
				{
					case 'R': // running
					case 'S': // interruptable sleeping
					case 'D': // uninterruptable sleeping
					case 'W': // paging
					case 'Z': // zombie
						controlledExecMapEntry.second->processWidget->show();
						if (controlledExecMapEntry.second->metaExec->name != "roscore")
						{
							controlledExecMapEntry.second->_processWidget->checkBox->setEnabled(true);
						}
						break;
					case 'T': // traced, or stopped
					case 'U': // unknown
					default:
						controlledExecMapEntry.second->processWidget->hide();
						break;
				}
			}
			return;
		}

		for (auto controlledExecMapEntry : this->controlledExecMap)
		{
			controlledExecMapEntry.second->processWidget->hide();
		}

		auto bundleMapEntry = this->parentProcessManager->parentPMControl->bundlesMap.find(text.toStdString());
		if (bundleMapEntry != this->parentProcessManager->parentPMControl->bundlesMap.end())
		{
			for (auto processParamSetPair : bundleMapEntry->second)
			{
				auto controlledExecMapEntry = this->controlledExecMap.find(processParamSetPair.first);
				if (controlledExecMapEntry != this->controlledExecMap.end())
				{
					controlledExecMapEntry->second->processWidget->show();
					if (processParamSetPair.second == controlledExecMapEntry->second->runningParamSet
							|| controlledExecMapEntry->second->runningParamSet == supplementary::ExecutableMetaData::UNKNOWN_PARAMS )
					{
						if (controlledExecMapEntry->second->metaExec->name != "roscore")
						{
							controlledExecMapEntry->second->_processWidget->checkBox->setEnabled(true);
						}
					}
					else
					{ // disable the checkbox, if the wrong bundle is selected
						controlledExecMapEntry->second->_processWidget->checkBox->setEnabled(false);
					}
				}
			}
		}

	}

	void ControlledRobot::addExec(QWidget* exec)
	{
		this->_robotProcessesWidget->verticalLayout->insertWidget(2, exec);
	}

	void ControlledRobot::removeExec(QWidget* exec)
	{
		this->_robotProcessesWidget->verticalLayout->removeWidget(exec);
	}

	void ControlledRobot::sendProcessCommand(vector<int> execIds, int newState)
	{
		this->parentProcessManager->sendProcessCommand(vector<int> {this->id}, execIds, newState);
	}
} /* namespace rqt_pm_control */
