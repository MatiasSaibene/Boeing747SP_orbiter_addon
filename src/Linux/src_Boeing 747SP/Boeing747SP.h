#ifndef __BOEING747SP_H
#define __BOEING747SP_H

#define STRICT 1
#include <cstdint>
#include "OrbiterAPI.h"
#include "Orbitersdk.h"
#include "VesselAPI.h"
#include "747SPdefinitions.h"

//Vessel parameters
const double B747SP_SIZE = 25.0;  //Mean radius in meters.

const double B747SP_EMPTYMASS = 147540; //Empty mass in kg.

const double B747SP_FUELMASS = 190630; //Fuel mass in kg.

const double B747SP_ISP = 2e4; //Fuel-specific impulse in m/s.

const double B747SP_MAXMAINTH = 500e3; //Max main thrust in kN.

const double LANDING_GEAR_OPERATING_SPEED = 0.06;

const double ENGINE_ROTATION_SPEED = 10;

const VECTOR3 B747SP_CS = {401.28, 868.57, 134.25};

const VECTOR3 B747SP_PMI = {204.50, 244.01, 135.71};

const double B747SP_VLIFT_C = 7.65; //Chord lenght in meters;

const double B747SP_VLIFT_S = 511; //Wing area in m^2;

const double B747SP_VLIFT_A = 7.0; //Wing aspect ratio;

const double B747SP_HLIFT_C = 8.2; //Chord lenght in meters;

const double B747SP_HLIFT_S = 61.4473; //Wing area in m^2;

const double B747SP_HLIFT_A = 6.1156; //Wing aspect ratio;

const double B747SP_STAB_C = 3.59; //Stabilizer chord lenght in meters.

const double B747SP_STAB_S = 39.0753; //Stabilizer wing area in m^2.

const double B747SP_STAB_A = 2; //Stabilizer wing aspect ratio.



//Define touchdown points
//For gear down
static const int ntdvtx_geardown = 14;
static TOUCHDOWNVTX tdvtx_geardown[ntdvtx_geardown] = {
    {(TDP1_Location), 5e6, 5e5, 1.6, 0.1},
    {(TDP2_Location), 5e6, 5e5, 3.0, 0.2},
    {(TDP3_Location), 5e6, 5e5, 3.0, 0.2},
    {(TDP4_Location), 5e6, 5e6, 3.0},
    {(TDP5_Location), 5e6, 5e6, 3.0},
    {(TDP6_Location), 5e6, 5e6, 3.0},
    {(TDP7_Location), 5e6, 5e6, 3.0},
    {(TDP8_Location), 5e6, 5e6, 3.0},
    {(TDP9_Location), 5e6, 5e6, 3.0},
    {(TDP10_Location), 5e6, 5e6, 3.0},
    {(TDP11_Location), 5e6, 5e6, 3.0},
    {(TDP12_Location), 5e6, 5e6, 3.0},
    {(TDP13_Location), 5e6, 5e6, 3.0},
    {(TDP14_Location), 5e6, 5e6, 3.0},
};


//For gear up
static const int ntdvtx_gearup = 11;
static TOUCHDOWNVTX tdvtx_gearup[ntdvtx_gearup] = {
    {(TDP4_Location), 5e6, 5e6, 3.0},
    {(TDP5_Location), 5e6, 5e6, 3.0},
    {(TDP6_Location), 5e6, 5e6, 3.0},
    {(TDP7_Location), 5e6, 5e6, 3.0},
    {(TDP8_Location), 5e6, 5e6, 3.0},
    {(TDP9_Location), 5e6, 5e6, 3.0},
    {(TDP10_Location), 5e6, 5e6, 3.0},
    {(TDP11_Location), 5e6, 5e6, 3.0},
    {(TDP12_Location), 5e6, 5e6, 3.0},
    {(TDP13_Location), 5e6, 5e6, 3.0},
    {(TDP14_Location), 5e6, 5e6, 3.0},
};

//B747100 class interface

class B747SP : public VESSEL4{

    public:
        enum LandingGearStatus{GEAR_DOWN, GEAR_UP, GEAR_DEPLOYING, GEAR_STOWING} landing_gear_status;

        B747SP(OBJHANDLE hVessel, int flightmodel);
        virtual ~B747SP();

        void DefineAnimations(void);
        void ActivateLandingGear(LandingGearStatus action);
        void SetGearDown(void);
        void UpdateLandingGearAnimation(double);

        double UpdateLvlEnginesContrail();

        void ParkingBrake();

        void NextSkin();
        void ChangeLivery();
        void ApplyLivery();

        void ActivateBeacons(void);

        void LightsControl(void);

        void clbkSetClassCaps(FILEHANDLE cfg) override;
        void clbkLoadStateEx(FILEHANDLE scn, void *vs) override;
        void clbkSaveState(FILEHANDLE scn) override;
        void clbkPreStep(double, double, double) override;
        void clbkPostStep(double, double, double) override;
        int clbkConsumeBufferedKey(int, bool, char *) override;

        void clbkVisualCreated(VISHANDLE vis, int refcount) override;
        void clbkVisualDestroyed (VISHANDLE vis, int refcount) override;

        VISHANDLE visual;
        MESHHANDLE b747sp_mesh;  //Mesh handle
        DEVMESHHANDLE b747sp_dmesh;  //Mesh template handle
        

    private:

        unsigned int anim_landing_gear;
        unsigned int anim_door;
        unsigned int anim_laileron;
        unsigned int anim_raileron;
        unsigned int anim_elevator;
        unsigned int anim_elevator_trim;
        unsigned int anim_rudder;
        unsigned int anim_engines;

        double lvlcontrailengines;
        double landing_gear_proc;
        double engines_proc;

        AIRFOILHANDLE lwing, rwing, lstabilizer, rstabilizer;
        CTRLSURFHANDLE hlaileron, hraileron;
        THRUSTER_HANDLE th_main[4], th_retro[4];
        THGROUP_HANDLE thg_main, thg_retro;
        BEACONLIGHTSPEC beacon[5];
        FILEHANDLE skinlist, skinlog;
        SURFHANDLE skin[5];
        char skinpath[64];
        LightEmitter *l1, *l2, *l3, *l4;

        COLOUR4 col_d = {0.9,0.8,1,0};
	    COLOUR4 col_s = {1.9,0.8,1,0};
	    COLOUR4 col_a = {0,0,0,0};

        const char fname[43] = "skins.txt";  //File where skin list is stored. Relative to ORBITER_ROOT.
        const char skindir[34] = "Boeing_747\\B747SP\\Skins\\";  //Path where actual skins are stored. Relative to ORBITER_ROOT.

        //Name of the textures to be applied.
        const char texname_fus[14] = "\\Fuselage.dds";
        const char texname_vs[25] = "\\Vertical_stabilizer.dds";
        const char texname_rw[15] = "Right_wing.dds";
        const char texname_eng[9] = "ENG1.dds";
        const char texname_lw[14] = "Left_wing.dds";

};

#endif