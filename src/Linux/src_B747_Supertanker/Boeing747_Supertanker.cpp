//Copyright (c) Matías Saibene
//Licenced under the MIT Licence

//==========================================
//          ORBITER MODULE: Boeing 747-Supertanker
//
//Boeing747_Supertanker.cpp
//Control module for Boeing 747-Supertanker vessel class
//
//==========================================

#define ORBITER_MODULE
#include "Boeing747_Supertanker.h"
#include <cstring>
#include <cstdio>
#include <algorithm>

bool parkingBrakeEnabled;
bool water_valve_open;
bool lights_on;
static int currentSkin = 0;
bool bGearIsDown;
bool engines_on;
int enginevalue;

// 1. vertical lift component

void VLiftCoeff (VESSEL *v, double aoa, double M, double Re, void *context, double *cl, double *cm, double *cd)
{
	const int nabsc = 9;
	static const double AOA[nabsc] = {-180*RAD,-60*RAD,-30*RAD, -15*RAD, 0*RAD,15*RAD,30*RAD,60*RAD,180*RAD};
	static const double CL[nabsc]  = {   0,    -0.56,   -0.56,   -0.16,  0.15,  0.46,  0.56,  0.56,  0.00};
	static const double CM[nabsc]  = {    0,    0.00,   0.00,     0.00,  0.00,  0.00,  0.00,  0.00,  0.00};


	int i;
	for (i = 0; i < nabsc-1 && AOA[i+1] < aoa; i++);
	if (i < nabsc - 1) {
		double f = (aoa - AOA[i]) / (AOA[i + 1] - AOA[i]);
		*cl = CL[i] + (CL[i + 1] - CL[i]) * f;  // aoa-dependent lift coefficient
		*cm = CM[i] + (CM[i + 1] - CM[i]) * f;  // aoa-dependent moment coefficient
	}
	else {
		*cl = CL[nabsc - 1];
		*cm = CM[nabsc - 1];
	}
	double saoa = sin(aoa);
	double pd = 0.015 + 0.4*saoa*saoa;  // profile drag
	*cd = pd + oapiGetInducedDrag (*cl, B747ST_VLIFT_A, 0.7) + oapiGetWaveDrag (M, 0.75, 1.0, 1.1, 0.04);
	// profile drag + (lift-)induced drag + transonic/supersonic wave (compressibility) drag
}

// 2. horizontal lift component (vertical stabilisers and body)

void HLiftCoeff (VESSEL *v, double beta, double M, double Re, void *context, double *cl, double *cm, double *cd)
{
	int i;
	const int nabsc = 8;
	static const double BETA[nabsc] = {-180*RAD,-135*RAD,-90*RAD,-45*RAD,45*RAD,90*RAD,135*RAD,180*RAD};
	static const double CL[nabsc]   = {       0,    +0.3,      0,   -0.3,  +0.3,     0,   -0.3,      0};
	for (i = 0; i < nabsc-1 && BETA[i+1] < beta; i++);
	if (i < nabsc - 1) {
		*cl = CL[i] + (CL[i + 1] - CL[i]) * (beta - BETA[i]) / (BETA[i + 1] - BETA[i]);
	}
	else {
		*cl = CL[nabsc - 1];
	}
	*cm = 0.0;
	*cd = 0.015 + oapiGetInducedDrag (*cl, B747ST_HLIFT_A, 0.6) + oapiGetWaveDrag (M, 0.75, 1.0, 1.1, 0.04);
}


//Constructor
B747ST::B747ST(OBJHANDLE hVessel, int flightmodel) : 
VESSEL4(hVessel, flightmodel){

	landing_gear_proc = 0.0;

	landing_gear_status = GEAR_DOWN;

    b747st_mesh = NULL;

    mhcockpit_mesh = NULL;

	DefineAnimations();

    skinpath[0] = '\0';
    for (int i = 0; i < 5; i++)
        skin[i] = 0;
}

//Destructor
B747ST::~B747ST(){

}

void B747ST::DefineAnimations(void){
    
    //Front landing gear

    static unsigned int FrontLandingGearGrp[1] = {Front_landing_gear_Id};
    static MGROUP_ROTATE FrontLandingGearRotate(
        0,
        FrontLandingGearGrp,
        1,
        (Axis_front_landing_gear_rotate_Location),
        _V(1, 0, 0),
        (float)(-110*RAD)
    );

    static unsigned int FrontLandingGearLeftDoorGrp[1] = {Front_landing_gear_left_door_Id};
    static MGROUP_ROTATE FrontLandingGearLeftDoor(
        0,
        FrontLandingGearLeftDoorGrp,
        1,
        (Axis_front_landing_gear_left_door_Location),
        _V(0, 0, 1),
        (float)(90*RAD)
    );

    static unsigned int FrontLandingGearRightDoorGrp[1] = {Front_landing_gear_right_door_Id};
    static MGROUP_ROTATE FrontLandingGearRightDoor(
        0,
        FrontLandingGearRightDoorGrp,
        1,
        (Axis_front_landing_gear_right_door_Location),
        _V(0, 0, 1),
        (float)(-90*RAD)
    );

    anim_landing_gear = CreateAnimation(0.0);

    AddAnimationComponent(anim_landing_gear, 0, 0.25, &FrontLandingGearRotate);
    AddAnimationComponent(anim_landing_gear, 0, 0.5, &FrontLandingGearLeftDoor);
    AddAnimationComponent(anim_landing_gear, 0, 0.5, &FrontLandingGearRightDoor);

    //Rear landing gears

    static unsigned int RearLeftWingLandingGearGrp[1] = {Rear_left_wing_landing_gear_Id};
    static MGROUP_ROTATE RearLeftWingLandingGearStow(
        0,
        RearLeftWingLandingGearGrp,
        1,
        (Axis_left_wing_landing_gear_Location),
        _V(0, 0, 1),
        (float)(90*RAD)
    );
    
    static unsigned int RearLeftWingLandingGearPartGrp [1] = {Rear_left_wing_landing_gear_part_Id};
    static MGROUP_ROTATE RearLeftWingLandingGearPart(
        0,
        RearLeftWingLandingGearPartGrp,
        1,
        (Axis_rear_left_wing_landing_gear_part_Location),
        _V(0, 0, 1),
        (float)(-90*RAD)
    );

    AddAnimationComponent(anim_landing_gear, 0, 0.5, &RearLeftWingLandingGearStow);
    AddAnimationComponent(anim_landing_gear, 0, 0.5, &RearLeftWingLandingGearPart);

    static unsigned int RearRightWingLandingGearGrp[1] = {Rear_right_wing_landing_gear_Id};
    static MGROUP_ROTATE RearRightWingLandingGearStow(
        0,
        RearRightWingLandingGearGrp,
        1,
        (Axis_right_wing_landing_gear_Location),
        _V(0, 0, 1),
        (float)(-90*RAD)
    );

    static unsigned int RearRightWingLandingGearPartGrp[1] = {Rear_right_wing_landing_gear_part_Id};
    static MGROUP_ROTATE RearRightWingLandingGearPart(
        0,
        RearRightWingLandingGearPartGrp,
        1,
        (Axis_rear_right_wing_landing_gear_part_Location),
        _V(0, 0, 1),
        (float)(90*RAD)
    );

    AddAnimationComponent(anim_landing_gear, 0, 0.5, &RearRightWingLandingGearStow);
    AddAnimationComponent(anim_landing_gear, 0, 0.5, &RearRightWingLandingGearPart);

    static unsigned int RearLeftLandingGearGrp[1] = {Rear_left_landing_gear_Id};
    static MGROUP_ROTATE RearLeftLandingGear(
        0,
        RearLeftLandingGearGrp,
        1,
        (Axis_left_landing_gear_Location),
        _V(1, 0, 0),
        (float)(-180*RAD)
    );

    static unsigned int RearRightLandingGearGrp[1] = {Rear_right_landing_gear_Id};
    static MGROUP_ROTATE RearRightLandingGear(
        0,
        RearRightLandingGearGrp,
        1,
        (Axis_right_landing_gear_Location),
        _V(1, 0, 0),
        (float)(-180*RAD)
    );

    AddAnimationComponent(anim_landing_gear, 0, 0.5, &RearLeftLandingGear);
    AddAnimationComponent(anim_landing_gear, 0, 0.5, &RearRightLandingGear);

    static unsigned int RearLeftLandingGearDoor1Grp[2] = {Rear_left_landing_gear_door1_Id, Rear_left_landing_gear_door2_Id};
    static MGROUP_ROTATE RearLeftLandingGearDoor1(
        0,
        RearLeftLandingGearDoor1Grp,
        2,
        (Axis_landing_gear_left_door_1_Location),
        _V(0, 0, 1),
        (float)(-90*RAD)
    );

    static unsigned int RearLeftLandingGearDoor2Grp[1] = {Rear_left_landing_gear_door2_Id};
    static MGROUP_ROTATE RearLeftLandingGearDoor2(
        0,
        RearLeftLandingGearDoor2Grp,
        1,
        (Axis_left_landing_gear_door2_Location),
        _V(0, 0, 1),
        (float)(-180*RAD)
    );

    AddAnimationComponent(anim_landing_gear, 0.25, 0.5, &RearLeftLandingGearDoor1);
    AddAnimationComponent(anim_landing_gear, 0.5, 0.75, &RearLeftLandingGearDoor2);

    static unsigned int RearRightLandingGearDoor1Grp[2] = {Rear_right_landing_gear_door1_Id, Rear_right_landing_gear_door2_Id};
    static MGROUP_ROTATE RearRightLandingGearDoor1(
        0,
        RearRightLandingGearDoor1Grp,
        2,
        (Axis_landing_gear_right_door_1_Location),
        _V(0, 0, 1),
        (float)(90*RAD)
    );

    static unsigned int RearRightLandingGearDoor2Grp[1] = {Rear_right_landing_gear_door2_Id};
    static MGROUP_ROTATE RearRightLandingGearDoor2(
        0,
        RearRightLandingGearDoor2Grp,
        1,
        (Axis_right_landing_gear_door2_Location),
        _V(0, 0, 1),
        (float)(180*RAD)
    );

    AddAnimationComponent(anim_landing_gear, 0.25, 0.5, &RearRightLandingGearDoor1);
    AddAnimationComponent(anim_landing_gear, 0.5, 0.75, &RearRightLandingGearDoor2);

    //Engines

    static unsigned int Engine1Grp[1] = {ENG1_blades_Id};
    static MGROUP_ROTATE Engine1(
        0,
        Engine1Grp,
        1,
        (ENG1_blades_Location),
        _V(0, 0, 1),
        (float)(360*RAD)
    );
    
    
    static unsigned int Engine2Grp[1] = {ENG2_blades_Id};
    static MGROUP_ROTATE Engine2(
        0,
        Engine2Grp,
        1,
        (ENG2_blades_Location),
        _V(0, 0, 1),
        (float)(360*RAD)
    );

    static unsigned int Engine3Grp[1] = {ENG3_blades_Id};
    static MGROUP_ROTATE Engine3(
        0,
        Engine3Grp,
        1,
        (ENG3_blades_Location),
        _V(0, 0, 1),
        (float)(360*RAD)
    );

    static unsigned int Engine4Grp[1] = {ENG4_blades_Id};
    static MGROUP_ROTATE Engine4(
        0,
        Engine4Grp,
        1,
        (ENG4_blades_Location),
        _V(0, 0, 1),
        (float)(360*RAD)
    );
    
    anim_engines = CreateAnimation(0.0);

    AddAnimationComponent(anim_engines, 0, 1, &Engine1);
    AddAnimationComponent(anim_engines, 0, 1, &Engine2);
    AddAnimationComponent(anim_engines, 0, 1, &Engine3);
    AddAnimationComponent(anim_engines, 0, 1, &Engine4);

    //Control surfaces

    static unsigned int LeftElevatorGrp[1] = {Left_elevator_trim_Id};
    static MGROUP_ROTATE LeftElevator(
        0,
        LeftElevatorGrp,
        1,
        (Axis_elevator_left_Location),
        _V(1, 0, 0.5),
        (float)(45*RAD)
    );

    static unsigned int RightElevatorGrp[1] = {Right_elevator_trim_Id};
    static MGROUP_ROTATE RightElevator(
        0,
        RightElevatorGrp,
        1,
        (Axis_elevator_right_Location),
        _V(1, 0, -0.5),
        (float)(45*RAD)
    );

    anim_elevator = CreateAnimation(0.5);
    AddAnimationComponent(anim_elevator, 0, 1, &LeftElevator);
    AddAnimationComponent(anim_elevator, 0, 1, &RightElevator);

    static unsigned int LeftElevatorTrimGrp[1] = {Left_elevator_trim_Id};
    static MGROUP_ROTATE LeftElevatorTrim(
        0,
        LeftElevatorTrimGrp,
        1,
        (Axis_elevator_left_Location),
        _V(1, 0, 0.5),
        (float)(22.5*RAD)
    );

    static unsigned int RightElevatorTrimGrp[1] = {Right_elevator_trim_Id};
    static MGROUP_ROTATE RightElevatorTrim(
        0,
        RightElevatorTrimGrp,
        1,
        (Axis_elevator_right_Location),
        _V(1, 0, -0.5),
        (float)(22.5*RAD)
    );

    anim_elevator_trim = CreateAnimation(0.5);
    AddAnimationComponent(anim_elevator_trim, 0, 1, &LeftElevatorTrim);
    AddAnimationComponent(anim_elevator_trim, 0, 1, &RightElevatorTrim);

    static unsigned int RudderGrp[1] = {Rudder_Id};
    static MGROUP_ROTATE Rudder(
        0,
        RudderGrp,
        1,
        (Axis_rudder_Location),
        _V(0, 1, -0.75),
        (float)(22.5*RAD)
    );
    anim_rudder = CreateAnimation(0.5);
    AddAnimationComponent(anim_rudder, 0, 1, &Rudder);

    static unsigned int LAileronGrp[1] = {LAileron_Id};
    static MGROUP_ROTATE LAileron(
        0,
        LAileronGrp,
        1,
        (Axis_laileron_Location),
        _V(-1, 0, -0.75),
        (float)(45*RAD)
    );

    static unsigned int RAileronGrp[1] = {Raileron_Id};
    static MGROUP_ROTATE RAileron(
        0,
        RAileronGrp,
        1,
        (Axis_raileron_Location),
        _V(1, 0, -0.75),
        (float)(45*RAD)
    );

    anim_laileron = CreateAnimation(0.5);
    anim_raileron = CreateAnimation(0.5);

    AddAnimationComponent(anim_laileron, 0, 1, &LAileron);
    AddAnimationComponent(anim_raileron, 0, 1, &RAileron);

        //Cockpit animations

    static unsigned int LYokeColumnGrp[2] = {LYoke_column_Id, LYoke_Id};
    static MGROUP_ROTATE LYokeColumn(
        uimesh_Cockpit,
        LYokeColumnGrp,
        2,
        (Axis_LYoke_column_Location),
        _V(-1, 0, 0),
        (float)(30*RAD)
    );

    static unsigned int RYokeColumnGrp[2] = {RYoke_column_Id, RYoke_Id};
    static MGROUP_ROTATE RYokeColumn(
        uimesh_Cockpit,
        RYokeColumnGrp,
        2,
        (Axis_RYoke_column_Location),
        _V(-1, 0, 0),
        (float)(30*RAD)
    );

    AddAnimationComponent(anim_elevator, 0, 1, &LYokeColumn);
    AddAnimationComponent(anim_elevator, 0, 1, &RYokeColumn);

    static unsigned int LYokeGrp[1] = {LYoke_Id};
    static MGROUP_ROTATE LYoke(
        uimesh_Cockpit,
        LYokeGrp,
        1,
        (LYoke_Location),
        _V(0, 0, -1),
        (float)(90*RAD)
    );

    static unsigned int RYokeGrp[1] = {RYoke_Id};
    static MGROUP_ROTATE RYoke(
        uimesh_Cockpit,
        RYokeGrp,
        1,
        (RYoke_Location),
        _V(0, 0, -1),
        (float)(90*RAD)
    );

    AddAnimationComponent(anim_laileron, 0, 1, &LYoke);
    AddAnimationComponent(anim_laileron, 0, 1, &RYoke);

    static unsigned int GearLeverGrp[1] = {Landing_gear_lever_Id};
    static MGROUP_ROTATE GearLever(
        uimesh_Cockpit,
        GearLeverGrp,
        1,
        (Axis_landing_gear_lever_Location),
        _V(1, 0, 0),
        (float)(90*RAD)
    );

    AddAnimationComponent(anim_landing_gear, 0, 0.1, &GearLever);
}

// Overloaded callback functions
// Set the capabilities of the vessel class
void B747ST::clbkSetClassCaps(FILEHANDLE cfg){

    //Physical vessel parameters
    SetSize(B747ST_SIZE);
    SetEmptyMass(B747ST_EMPTYMASS);
	SetCrossSections(B747ST_CS);
	SetPMI(B747ST_PMI);
	SetMaxWheelbrakeForce(89e3);
	SetRotDrag(_V(10, 10, 2.5));
	
	PROPELLANT_HANDLE JET_A1 = CreatePropellantResource(B747ST_FUELMASS);

	th_main[0] = CreateThruster((ENG1_Location), _V(0, 0, 1), B747ST_MAXMAINTH, JET_A1, B747ST_ISP);
    th_main[1] = CreateThruster((ENG2_Location), _V(0, 0, 1), B747ST_MAXMAINTH, JET_A1, B747ST_ISP);
    th_main[2] = CreateThruster((ENG3_Location), _V(0, 0, 1), B747ST_MAXMAINTH, JET_A1, B747ST_ISP);
    th_main[3] = CreateThruster((ENG4_Location), _V(0, 0, 1), B747ST_MAXMAINTH, JET_A1, B747ST_ISP);
    thg_main = CreateThrusterGroup(th_main, 4, THGROUP_MAIN);

    th_retro[0] = CreateThruster((ENG1_Location), _V(0, 0, -1), (B747ST_MAXMAINTH/4), JET_A1, B747ST_ISP);
    th_retro[1] = CreateThruster((ENG2_Location), _V(0, 0, -1), (B747ST_MAXMAINTH/4), JET_A1, B747ST_ISP);
    th_retro[2] = CreateThruster((ENG3_Location), _V(0, 0, -1), (B747ST_MAXMAINTH/4), JET_A1, B747ST_ISP);
    th_retro[3] = CreateThruster((ENG4_Location), _V(0, 0, -1), (B747ST_MAXMAINTH/4), JET_A1, B747ST_ISP);
    thg_retro = CreateThrusterGroup(th_retro, 4, THGROUP_RETRO);

    PROPELLANT_HANDLE WATER = CreatePropellantResource(B747ST_WATERMASS);
    wdisch[0] = CreateThruster((Water_discharge1_Location), _V(0, 1, 0), B747ST_MAXWATERTH, WATER, B747ST_WISP);
    wdisch[1] = CreateThruster((Water_discharge2_Location), _V(0, 1, 0), B747ST_MAXWATERTH, WATER, B747ST_WISP);
    wdisch_main = CreateThrusterGroup(wdisch, 2, THGROUP_USER);

	//Contrail effect on engines
    static PARTICLESTREAMSPEC engines_contrails = {
        0, 0.5, .95, 120, 0.03, 10.0, 5, 3.0, 
        PARTICLESTREAMSPEC::EMISSIVE,
		PARTICLESTREAMSPEC::LVL_PLIN, -1.0, 25.0,
		PARTICLESTREAMSPEC::ATM_PLIN, 
    };

    static PARTICLESTREAMSPEC water_contrail = {
        0, 2.0, 20, 200, 0.05, 2.5, 8, 1.0,
        PARTICLESTREAMSPEC::EMISSIVE,
		PARTICLESTREAMSPEC::LVL_SQRT, 0, 1,
		PARTICLESTREAMSPEC::ATM_PLOG, 1e-5, 0.1
    };

	AddParticleStream(&engines_contrails, (ENG1_Location), _V(0, 0, -1), &lvlcontrailengines);
    AddParticleStream(&engines_contrails, (ENG2_Location), _V(0, 0, -1), &lvlcontrailengines);
    AddParticleStream(&engines_contrails, (ENG3_Location), _V(0, 0, -1), &lvlcontrailengines);
    AddParticleStream(&engines_contrails, (ENG4_Location), _V(0, 0, -1), &lvlcontrailengines);

    AddExhaustStream(wdisch[0], (Water_discharge1_Location), &water_contrail);
    AddExhaustStream(wdisch[1], (Water_discharge2_Location), &water_contrail);

	lwing = CreateAirfoil3(LIFT_VERTICAL, (Left_wing_Location), VLiftCoeff, 0, B747ST_VLIFT_C, (B747ST_VLIFT_S*4), B747ST_VLIFT_A);

    rwing = CreateAirfoil3(LIFT_VERTICAL,(Right_wing_Location), VLiftCoeff, 0, B747ST_VLIFT_C,(B747ST_VLIFT_S*4), B747ST_VLIFT_A);

    lstabilizer = CreateAirfoil3(LIFT_VERTICAL, (Left_stabilizer_Location), VLiftCoeff, 0, B747ST_STAB_C, B747ST_STAB_S, B747ST_STAB_A);

    rstabilizer = CreateAirfoil3(LIFT_VERTICAL, (Right_stabilizer_Location), VLiftCoeff, 0, B747ST_STAB_C, B747ST_STAB_S, B747ST_STAB_A);
    
    CreateAirfoil3(LIFT_HORIZONTAL, (Rudder_Location), HLiftCoeff, 0, B747ST_HLIFT_C, B747ST_HLIFT_S, B747ST_HLIFT_A);

    hlaileron = CreateControlSurface3(AIRCTRL_AILERON,8.3696, 1.7, (LAileron_Location), AIRCTRL_AXIS_AUTO, 1.0, anim_laileron);
    hraileron = CreateControlSurface3(AIRCTRL_AILERON, 8.3696, 1.7, (Raileron_Location), AIRCTRL_AXIS_AUTO, 1.0, anim_raileron);

    CreateControlSurface3(AIRCTRL_ELEVATOR, (14.4997*2), 1.7, (Left_elevator_trim_Location), AIRCTRL_AXIS_AUTO, 1.0, anim_elevator);
    CreateControlSurface3(AIRCTRL_ELEVATOR, (14.4997*2), 1.7, (Right_elevator_trim_Location), AIRCTRL_AXIS_AUTO, 1.0, anim_elevator);

    CreateControlSurface3(AIRCTRL_ELEVATORTRIM, (14.4997*2), 1.7, (Left_elevator_trim_Location), AIRCTRL_AXIS_AUTO, 1.0, anim_elevator_trim);
    CreateControlSurface3(AIRCTRL_ELEVATORTRIM, (14.4997*2), 1.7, (Right_elevator_trim_Location), AIRCTRL_AXIS_AUTO, 1.0, anim_elevator_trim);

    CreateControlSurface3(AIRCTRL_RUDDER, 20.6937, 1.7, (Rudder_Location), AIRCTRL_AXIS_AUTO, 1.0, anim_rudder);

    //Define beacons

    static VECTOR3 beaconpos_green[2] = { {Beacon2_right_wing_Location}, {Beacon3_upper_deck_Location}};

    static VECTOR3 beaconpos_red[2] = {{Beacon1_left_wing_Location}, {Beacon4_belly_landing_gear_Location}};

    static VECTOR3 beaconpos_white[1] = {Beacon5_APU_Location};

    static VECTOR3 beaconcol_green = {0, 1, 0};
    static VECTOR3 beaconcol_red = {1, 0, 0};
    static VECTOR3 beaconcol_white = {1, 1, 1};

    for(int i = 0; i < 2; i++){
		beacongreen[i].shape = BEACONSHAPE_STAR;
		beacongreen[i].pos = beaconpos_green+i;
		beacongreen[i].col = &beaconcol_green;
		beacongreen[i].size = 1;
		beacongreen[i].falloff = 0.4;
		beacongreen[i].period = 1;
		beacongreen[i].duration = 0.1;
		beacongreen[i].tofs = 0.2;
		beacongreen[i].active = false;
		AddBeacon(beacongreen+i);
	}

    for(int i = 0; i < 2; i++){
		beaconred[i].shape = BEACONSHAPE_STAR;
		beaconred[i].pos = beaconpos_red+i;
		beaconred[i].col = &beaconcol_red;
		beaconred[i].size = 1;
		beaconred[i].falloff = 0.4;
		beaconred[i].period = 1;
		beaconred[i].duration = 0.1;
		beaconred[i].tofs = 0.2;
		beaconred[i].active = false;
		AddBeacon(beaconred+i);
	}

    for(int i = 0; i < 1; i++){
		beaconwhite[i].shape = BEACONSHAPE_STAR;
		beaconwhite[i].pos = beaconpos_white+i;
		beaconwhite[i].col = &beaconcol_white;
		beaconwhite[i].size = 1;
		beaconwhite[i].falloff = 0.4;
		beaconwhite[i].period = 1;
		beaconwhite[i].duration = 0.1;
		beaconwhite[i].tofs = 0.2;
		beaconwhite[i].active = false;
		AddBeacon(beaconwhite+i);
	}

    //Add the mesh
    SetMeshVisibilityMode (AddMesh (b747st_mesh = oapiLoadMeshGlobal ("Boeing747\\Boeing_747_Supertanker")), MESHVIS_EXTERNAL);

    //Add the mesh for the cockpit
    SetMeshVisibilityMode(AddMesh(mhcockpit_mesh = oapiLoadMeshGlobal("Boeing747\\Boeing_747_cockpit")), MESHVIS_VC);

}

void B747ST::clbkVisualCreated(VISHANDLE vis, int refcount){

    visual = vis;

    b747st_dmesh = GetDevMesh(vis, 0);

    ApplyLivery();


}

void B747ST::clbkVisualDestroyed(VISHANDLE vis, int refcount){

    visual = NULL;
    b747st_dmesh = NULL;

}

void B747ST::NextSkin() {
    if (currentSkin >= 2) {
        currentSkin = 0;
    }

    ChangeLivery();
    currentSkin++;
}

void B747ST::ChangeLivery() {
    
    char skinname[256];
    char completedir_fus[256];
    char completedir_vs[256];
    char completedir_rw[256];
    char completedir_eng[256];
    char completedir_lw[256];
    const char SKINLIST[][6] = {"SKIN1", "SKIN2"};
    
    skinlist = oapiOpenFile(fname, FILE_IN, ROOT);
    oapiReadItem_string(skinlist, SKINLIST[currentSkin], skinname);
    

    strcpy(completedir_fus, skindir);
    strcat(completedir_fus, skinname);
    strcat(completedir_fus, texname_fus);

    strcpy(completedir_vs, skindir);
    strcat(completedir_vs, skinname);
    strcat(completedir_vs, texname_vs);

    strcpy(completedir_rw, skindir);
    strcat(completedir_rw, skinname);
    strcat(completedir_rw, texname_rw);

    strcpy(completedir_eng, skindir);
    strcat(completedir_eng, skinname);
    strcat(completedir_eng, texname_eng);

    strcpy(completedir_lw, skindir);
    strcat(completedir_lw, skinname);
    strcat(completedir_lw, texname_lw);

    skin[0] = oapiLoadTexture(completedir_fus);
    skin[1] = oapiLoadTexture(completedir_vs);
    skin[2] = oapiLoadTexture(completedir_rw);
    skin[3] = oapiLoadTexture(completedir_eng);
    skin[4] = oapiLoadTexture(completedir_lw);


    ApplyLivery();
}

void B747ST::ApplyLivery(){

    if(!b747st_dmesh) return;

    if(skin[0]) oapiSetTexture(b747st_dmesh, 1, skin[0]);

    if(skin[1]) oapiSetTexture(b747st_dmesh, 2, skin[1]);

    if(skin[2]) oapiSetTexture(b747st_dmesh, 3, skin[2]);

    if(skin[3]) oapiSetTexture(b747st_dmesh, 4, skin[3]);

    if(skin[4]) oapiSetTexture(b747st_dmesh, 9, skin[4]);

}


void B747ST::LightsControl(void){

    if(!lights_on){
        l1 = AddSpotLight((LIGHT1_Location), _V(0, 0, 1), 100000, 1e-3, 0, 2e-3, 25*RAD, 45*RAD, col_d, col_s, col_a);
        l2 = AddSpotLight((LIGHT2_Location), _V(0, 0, 1), 100000, 1e-3, 0, 2e-3, 25*RAD, 45*RAD, col_d, col_s, col_a);
        l3 = AddSpotLight((LIGHT3_Location), _V(0, 0, 1), 100000, 1e-3, 0, 2e-3, 25*RAD, 45*RAD, col_d, col_s, col_a);
        l4 = AddSpotLight((LIGHT4_Location), _V(0, 0, 1), 100000, 1e-3, 0, 2e-3, 25*RAD, 45*RAD, col_d, col_s, col_a);

        cpl1 = AddPointLight((PL1_Location), 1, 0.15, 0, 0.15, ccol_d, ccol_s, ccol_a);
        cpl1->SetVisibility(LightEmitter::VIS_COCKPIT);
        cpl2 = AddPointLight((PL2_Location), 1, 0.15, 0, 0.15, ccol_d, ccol_s, ccol_a);
        cpl2->SetVisibility(LightEmitter::VIS_COCKPIT);

        lights_on = true;
    } else {
        DelLightEmitter(l1);
        DelLightEmitter(l2);
        DelLightEmitter(l3);
        DelLightEmitter(l4);

        DelLightEmitter(cpl1);
        DelLightEmitter(cpl2);

        lights_on = false;
    }
}

bool B747ST::clbkLoadVC(int id){

    static VCMFDSPEC mfds_1 = {1, MFD1_Id};
    oapiVCRegisterMFD(MFD_LEFT, &mfds_1);

    static VCMFDSPEC mfds_2 = {1, MFD2_Id};
    oapiVCRegisterMFD(MFD_RIGHT, &mfds_2);

    switch(id){
        case 0 : //Commander
            SetCameraOffset(Captains_camera_Location);
            SetCameraDefaultDirection(_V(0, 0, 1));
            SetCameraRotationRange(RAD*120, RAD*120, RAD*60, RAD*60);
            oapiVCSetNeighbours(-1, 1, -1, 2);
        break;

        case 1 : //First officer
            SetCameraOffset(First_officer_camera_Location);
            SetCameraDefaultDirection(_V(0, 0, 1));
            SetCameraRotationRange(RAD*120, RAD*120, RAD*60, RAD*60);
            oapiVCSetNeighbours(0, -1, -1, 2);
        break;

        case 2: //Engineer
            SetCameraOffset(Engineer_camera_Location);
            SetCameraDefaultDirection(_V(1, 0, 0));
            SetCameraRotationRange(RAD*120, RAD*120, RAD*60, RAD*60);
            oapiVCSetNeighbours(1, -1, -1, 3);
        break;
    }

    return true; 

}

void B747ST::DischargeWater(){

    if(!water_valve_open){
        SetThrusterGroupLevel(wdisch_main, 1);
        water_valve_open = true;
    } else {
        SetThrusterGroupLevel(wdisch_main, 0.0);
        water_valve_open = false;
    }

}

void B747ST::ParkingBrake(){

    if(!parkingBrakeEnabled){
        SetWheelbrakeLevel(1, 0, true);
        parkingBrakeEnabled = true;
    } else {
        SetWheelbrakeLevel(0, 0, true);
        parkingBrakeEnabled = false;
    }

}

void B747ST::ActivateBeacons(){

    for(int i = 0; i < 2; i++){
		if(!beacongreen[i].active){
				beacongreen[i].active = true;
		} else {
				beacongreen[i].active = false;
		}
	}

    for(int i = 0; i < 2; i++){
		if(!beaconred[i].active){
				beaconred[i].active = true;
		} else {
				beaconred[i].active = false;
		}
	}

    for(int i = 0; i < 1; i++){
		if(!beaconwhite[i].active){
				beaconwhite[i].active = true;
		} else {
				beaconwhite[i].active = false;
		}
	}

}

void B747ST::EnginesAutostart(void){

    engines_on = true;
    enginevalue = 1;
    m_pXRSound->PlayWav(engines_start);
    
}

void B747ST::EnginesAutostop(void){

    engines_on = false;
    enginevalue = 0;
    m_pXRSound->PlayWav(engines_shutdown);
    
}

void B747ST::UpdateEnginesStatus(){

    if(engines_on == true){
        thg_main = CreateThrusterGroup(th_main, 4, THGROUP_MAIN);

        thg_retro = CreateThrusterGroup(th_retro, 4, THGROUP_RETRO);

        pwr = GetThrusterLevel(thg_main);

    } else if (engines_on == false){
        DelThrusterGroup(thg_main);
        DelThrusterGroup(thg_retro);
        pwr = 0;
    }
}

int B747ST::clbkConsumeBufferedKey(int key, bool down, char *kstate){

    if(key == OAPI_KEY_G && down){
        SetGearDown();
        return 1;
    }
    if(key == OAPI_KEY_NUMPADENTER && down){
        ParkingBrake();
        return 1;
    }
    if(key == OAPI_KEY_W && down){
        DischargeWater();
        return 1;
    }
    if(key == OAPI_KEY_F && down){
        LightsControl();
        return 1;
    }
    if(key == OAPI_KEY_B && down){
        ActivateBeacons();
        return 1;
    }
    if(key == OAPI_KEY_V && down){
        NextSkin();
        return 1;
    }
    if(down){
        if(KEYMOD_CONTROL(kstate)){
            switch(key){
                case OAPI_KEY_A:
                EnginesAutostart();
                return 1;

                case OAPI_KEY_E:
                EnginesAutostop();
                return 1;
            }
        }
    }
    return 0;
}

//Load landing gear status from scenario file
void B747ST::clbkLoadStateEx(FILEHANDLE scn, void *vs){
    
    char *line;

    while(oapiReadScenario_nextline(scn, line)){
        if(!strncasecmp(line, "GEAR", 4)){
            sscanf(line+4, "%d%lf", (int *)&landing_gear_status, &landing_gear_proc);
            SetAnimation(anim_landing_gear, landing_gear_proc);
            if (landing_gear_proc == 1.0){
                bGearIsDown = true;
            } else {
                bGearIsDown = false;
            }
        } else if (!strncasecmp(line, "ENGINES", 7)){
            sscanf(line+7, "%d", &enginevalue);
            if(enginevalue == 1){
                engines_on = true;
            } else {
                engines_on = false;
            }
        } else {
            ParseScenarioLineEx(line, vs);
        }
    }
}

void B747ST::clbkSaveState(FILEHANDLE scn){

    char cbuf[256];

    SaveDefaultState(scn);
    sprintf(cbuf, "%d %0.4f", landing_gear_status, landing_gear_proc);
    oapiWriteScenario_string(scn, "GEAR", cbuf);
    
    oapiWriteScenario_int(scn, "ENGINES", enginevalue);
    
}

//////////Logic for animations
void B747ST::SetGearDown(void){
    ActivateLandingGear((landing_gear_status == GEAR_DOWN || landing_gear_status == GEAR_DEPLOYING) ?
        GEAR_STOWING : GEAR_DEPLOYING);
}

void B747ST::ActivateLandingGear(LandingGearStatus action){
    landing_gear_status = action;
}

void B747ST::UpdateLandingGearAnimation(double simdt) {
    if (landing_gear_status >= GEAR_DEPLOYING) {
        double da = simdt * LANDING_GEAR_OPERATING_SPEED;
        if (landing_gear_status == GEAR_DEPLOYING) {
            if (landing_gear_proc > 0.0) landing_gear_proc = std::max(0.0, landing_gear_proc - da);
            else landing_gear_status = GEAR_DOWN;
            SetTouchdownPoints(tdvtx_geardown, ntdvtx_geardown);
            bGearIsDown = true;
        } else {
            if (landing_gear_proc < 1.0) landing_gear_proc = std::min(1.0, landing_gear_proc + da);
            else landing_gear_status = GEAR_UP;
            SetTouchdownPoints(tdvtx_gearup, ntdvtx_gearup);
            bGearIsDown = false;
        }
        SetAnimation(anim_landing_gear, landing_gear_proc);
    }
}

void B747ST::UpdateGearStatus(void){
    if(!bGearIsDown){
        SetTouchdownPoints(tdvtx_geardown, ntdvtx_geardown);
        SetNosewheelSteering(true);
    } else if (bGearIsDown){
        SetTouchdownPoints(tdvtx_gearup, ntdvtx_gearup);
        SetNosewheelSteering(false);
    }
}

double B747ST::UpdateLvlEnginesContrail(){
    double machnumber = GetMachNumber();
    double altitude = GetAltitude();

    if((machnumber > 0.5) && ((altitude > 10000) && (altitude < 15000))){
        return 1.0;
    } else {
        return 0.0;
    }

}

void B747ST::clbkPostStep(double simt, double simdt, double mjd){
    UpdateLandingGearAnimation(simdt);
    lvlcontrailengines = UpdateLvlEnginesContrail();
    UpdateEnginesStatus();
}

void B747ST::clbkPostCreation(){

    UpdateGearStatus();

    m_pXRSound = XRSound::CreateInstance(this);

    m_pXRSound->LoadWav(engines_start, "XRSound\\Boeing747\\747_APU_Start.wav", XRSound::PlaybackType::BothViewFar);

    m_pXRSound->LoadWav(engines_shutdown, "XRSound\\Boeing747\\747_APU_Shutdown.wav", XRSound::PlaybackType::BothViewFar);

    m_pXRSound->LoadWav(XRSound::MainEngines, "XRSound\\Boeing747\\roar.wav", XRSound::PlaybackType::BothViewFar);

    m_pXRSound->LoadWav(XRSound::RetroEngines, "XRSound\\Boeing747\\roar.wav", XRSound::PlaybackType::BothViewFar);

    m_pXRSound->LoadWav(cabin_ambiance, "XRSound\\Boeing747\\747_cabin_ambiance.wav", XRSound::PlaybackType::InternalOnly);

    m_pXRSound->SetDefaultSoundEnabled(XRSound::MainEngines, "XRSound\\Boeing747\\roar.wav");

    m_pXRSound->SetDefaultSoundEnabled(XRSound::RetroEngines, "XRSound\\Boeing747\\roar.wav");

    m_pXRSound->LoadWav(gear_movement, "XRSound\\Default\\Gear Whine.wav", XRSound::PlaybackType::BothViewMedium);

}

void B747ST::clbkPreStep(double simt, double simdt, double mjd){

    double grndspd = GetGroundspeed();
    double pwr = GetThrusterLevel(th_main);
    double prp = GetAnimation(anim_engines);
    double msimdt = simdt * ENGINE_ROTATION_SPEED;
    double da = msimdt * 0.1 + (pwr * 0.1);

    engines_proc = prp + da;

    if(prp < 1){
        SetAnimation(anim_engines, engines_proc);
    } else {
        SetAnimation(anim_engines, 0.0);
    }
}

///////////////

DLLCLBK void InitModule(MODULEHANDLE hModule){


}

DLLCLBK void ExitModule(MODULEHANDLE *hModule){

}



///////////////Vessel initialization

DLLCLBK VESSEL *ovcInit(OBJHANDLE hvessel, int flightmodel){
    
	return new B747ST(hvessel, flightmodel);

}

/////////////Vessel memory cleanup
DLLCLBK void ovcExit(VESSEL *vessel){
    
	if(vessel) delete(B747ST*)vessel;
	
}