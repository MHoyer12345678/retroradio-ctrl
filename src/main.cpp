/*
 * main.cpp
 *
 *  Created on: 22.10.2017
 *      Author: joe
 */
#include "RetroradioController.h"

#include "cpp-app-utils/Logger.h"
using namespace retroradio_controller;

int main (int argc, char **argv)
{
	int result;

	RetroradioController *retroradioControllerInstance=RetroradioController::Instance();
	if (retroradioControllerInstance->Init(argc,argv))
		retroradioControllerInstance->Run();

	result=retroradioControllerInstance->GetReturnCode();

	retroradioControllerInstance->DeInit();

	return result;
}




