
/* 
 * File:   Motor.cpp
 * Author: Richard Greene
 * Implements a motor controlled via I2C
 * Created on March 13, 2014, 5:51 PM
 */

#include <Motor.h>
#include <Settings.h>
    
/// Public constructor, base class opens I2C connection and sets slave address
Motor::Motor(unsigned char slaveAddress) :
I2C_Device(slaveAddress)
{
 
}

/// Disables motors
Motor::~Motor() 
{
    DisableMotors();
}    

/// Send a single command to the motor controller.  
bool Motor::SendCommand(MotorCommand command)
{
    return command.Send(this);
}

/// Send a set of commands to the motor controller.  Returns false immediately 
/// if any of the commands cannot be sent.
bool Motor::SendCommands(std::vector<MotorCommand> commands)
{
    for(int i = 0; i < commands.size(); i++)
        if(!commands[i].Send(this))
            return false;
    
    return true;
}

/// Perform a software reset of the motor controller.
bool Motor::Reset()
{
    return(MotorCommand(MC_GENERAL_REG, MC_RESET).Send(this));
}

/// Enable (engage) both motors.  Return false if either can't be enabled.
bool Motor::EnableMotors()
{
    return(MotorCommand(MC_ROT_ACTION_REG, MC_ENABLE).Send(this) &&
           MotorCommand(MC_Z_ACTION_REG, MC_ENABLE).Send(this));
}

/// Disable (disengage) both motors.  Return false if either can't be disabled.
bool Motor::DisableMotors()
{
    return(MotorCommand(MC_ROT_ACTION_REG, MC_DISABLE).Send(this) &&
           MotorCommand(MC_Z_ACTION_REG, MC_DISABLE).Send(this));    
}

/// Pause the current motor command(s) in progress.
bool Motor::Pause()
{
    return(MotorCommand(MC_GENERAL_REG, MC_PAUSE).Send(this));
}

/// Resume the  motor command(s) pending at last pause.
bool Motor::Resume()
{
    return(MotorCommand(MC_GENERAL_REG, MC_RESUME).Send(this));
}

/// Clear pending motor command(s).  Typical use would be after a pause, to
/// implement a cancel.
bool Motor::ClearCommandQueue()
{
    return(MotorCommand(MC_GENERAL_REG, MC_CLEAR).Send(this));
}

/// Prepare the motor controller to accept further commands.
bool Motor::Initialize()
{
    std::vector<MotorCommand> commands;
    
    // TODO: use settings for all numeric values
    // set up parameters applying to all Z motions
    commands.push_back(MotorValueCommand(MC_Z_SETTINGS_REG, MC_Z_SCREW_PITCH,
                                         SETTINGS.GetInt(Z_SCREW_PITCH)));
    commands.push_back(MotorValueCommand(MC_Z_SETTINGS_REG, MC_Z_MAX_TRAVEL, 
                                         SETTINGS.GetInt(Z_MAX_TRAVEL)));
    commands.push_back(MotorValueCommand(MC_Z_SETTINGS_REG, MC_GEAR_RATIO, 
                                         SETTINGS.GetInt(Z_GEAR_RATIO)));
    commands.push_back(MotorValueCommand(MC_Z_SETTINGS_REG, MC_MICROSTEPPING, 
                                         SETTINGS.GetInt(Z_MICRO_STEP)));
    commands.push_back(MotorValueCommand(MC_Z_SETTINGS_REG, MC_START_SPEED, 
                                         SETTINGS.GetInt(Z_START_SPEED)));

    // set up parameters applying to all rotations
    commands.push_back(MotorValueCommand(MC_ROT_SETTINGS_REG, MC_GEAR_RATIO, 
                                         SETTINGS.GetInt(R_GEAR_RATIO)));
    commands.push_back(MotorValueCommand(MC_ROT_SETTINGS_REG, MC_MICROSTEPPING, 
                                         SETTINGS.GetInt(R_MICRO_STEP)));
    commands.push_back(MotorValueCommand(MC_ROT_SETTINGS_REG, MC_START_SPEED, 
                                         SETTINGS.GetInt(R_START_SPEED)));

    // enable the motors
    commands.push_back(MotorCommand(MC_ROT_ACTION_REG, MC_ENABLE));
    commands.push_back(MotorCommand(MC_Z_ACTION_REG, MC_ENABLE));
    
    // request an interrupt when these commands are completed
    commands.push_back(MotorCommand(MC_GENERAL_REG, MC_INTERRUPT));

    return SendCommands(commands);        
}


/// Move the motors to their home position.
bool Motor::GoHome()
{
    std::vector<MotorCommand> commands;
    
    // set rotation parameters
    commands.push_back(MotorValueCommand(MC_ROT_SETTINGS_REG, MC_ACCELERATION, 
                                         SETTINGS.GetInt(R_HOMING_ACCEL)));
    commands.push_back(MotorValueCommand(MC_ROT_SETTINGS_REG, MC_SPEED, 
                                         SETTINGS.GetInt(R_HOMING_SPEED)));
    // rotate to the home position
    commands.push_back(MotorValueCommand(MC_ROT_ACTION_REG, MC_MOVE, 0));    
    // rotate 60 degrees to the start position
    commands.push_back(MotorValueCommand(MC_ROT_ACTION_REG, MC_MOVE, 
                                         TRAY_START_ANGLE));
    
    // set Z motion parameters
    commands.push_back(MotorValueCommand(MC_Z_SETTINGS_REG, MC_ACCELERATION, 
                                         SETTINGS.GetInt(Z_HOMING_ACCEL)));
    commands.push_back(MotorValueCommand(MC_Z_SETTINGS_REG, MC_SPEED, 
                                         SETTINGS.GetInt(Z_HOMING_SPEED)));
    // go to the Z axis upper limit, i.e the home position
    commands.push_back(MotorValueCommand(MC_Z_ACTION_REG, MC_MOVE, 0));
    // request an interrupt when these commands are completed
    commands.push_back(MotorCommand(MC_GENERAL_REG, MC_INTERRUPT));
    
    return SendCommands(commands);
}

/// Separate the current layer and go to the position for the next layer. 
bool Motor::GoToNextLayer()
{
    std::vector<MotorCommand> commands;

    // rotate the previous layer from the PDMS
    commands.push_back(MotorValueCommand(MC_ROT_SETTINGS_REG, MC_ACCELERATION, 
                                         SETTINGS.GetInt(R_SEPARATING_ACCEL)));
    commands.push_back(MotorValueCommand(MC_ROT_SETTINGS_REG, MC_DECELERATION, 
                                         SETTINGS.GetInt(R_SEPARATING_DECEL)));
    commands.push_back(MotorValueCommand(MC_ROT_SETTINGS_REG, MC_SPEED, 
                                         SETTINGS.GetInt(R_SEPARATING_SPEED)));
    commands.push_back(MotorValueCommand(MC_ROT_SETTINGS_REG, MC_MOVE, 
                                         -1 * TRAY_START_ANGLE));
    // lift the build platform
    commands.push_back(MotorValueCommand(MC_Z_SETTINGS_REG, MC_ACCELERATION, 
                                         SETTINGS.GetInt(Z_SEPARATING_ACCEL)));
    commands.push_back(MotorValueCommand(MC_Z_SETTINGS_REG, MC_DECELERATION, 
                                         SETTINGS.GetInt(Z_SEPARATING_DECEL)));
    commands.push_back(MotorValueCommand(MC_Z_SETTINGS_REG, MC_SPEED, 
                                         SETTINGS.GetInt(Z_SEPARATING_SPEED)));
    int deltaZ = SETTINGS.GetInt(Z_SEPARATING_HEIGHT);
    commands.push_back(MotorValueCommand(MC_Z_SETTINGS_REG, MC_MOVE, 
                                         deltaZ));
    
    // rotate back to the PDMS
    commands.push_back(MotorValueCommand(MC_ROT_SETTINGS_REG, MC_MOVE, 
                                         TRAY_START_ANGLE));
    
    // and lower into position to expose the next layer
    commands.push_back(MotorValueCommand(MC_Z_SETTINGS_REG, MC_MOVE, 
                         SETTINGS.GetInt(LAYER_THICKNESS) - deltaZ));
    
    // request an interrupt when these commands are completed
    commands.push_back(MotorCommand(MC_GENERAL_REG, MC_INTERRUPT));
    
    return SendCommands(commands);
}