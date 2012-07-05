#ifndef CVR_PARTICLE_DREAMS_H
#define CVR_PARTICLE_DREAMS_H

#include <cvrKernel/CVRPlugin.h>
#include <cvrKernel/CVRViewer.h>
#include <cvrMenu/SubMenu.h>
#include <cvrMenu/MenuCheckbox.h>

#include "CudaParticle.h"
#include "PDObject.h"

#include <osg/Geometry>
#include <osg/Geode>
#include <OpenThreads/Mutex>

class ParticleDreams : public cvr::CVRPlugin, public cvr::MenuCallback, public cvr::PerContextCallback
{
    public:
        ParticleDreams();
        virtual ~ParticleDreams();

        bool init();

        void menuCallback(cvr::MenuItem * item);
        void preFrame();
        bool processEvent(cvr::InteractionEvent * event);

        virtual void perContextCallback(int contextid) const;
    protected:
        void initPart();
        void initGeometry();
        void initSound();

        void updateHand();

        void pdata_init_age(int mAge);
        void pdata_init_velocity(float vx,float vy,float vz);
	void pdata_init_rand();
        void copy_reflector(int source, int dest);
        void copy_injector( int sorce, int destination);
        int load6wallcaveWalls(int firstRefNum);

        void scene_data_0_host();
        void scene_data_1_host();
        void scene_data_2_host();
        void scene_data_3_host();
        void scene_data_4_host();

        void scene_data_0_context(int contextid) const;
        void scene_data_1_context(int contextid) const;
        void scene_data_2_context(int contextid) const;
        void scene_data_3_context(int contextid) const;
        void scene_data_4_context(int contextid) const;

        cvr::SubMenu * _myMenu;
        cvr::MenuCheckbox * _enable;

        PDObject * _particleObject;
        osg::ref_ptr<osg::Geometry> _particleGeo;
        osg::ref_ptr<osg::Geode> _particleGeode;
        osg::ref_ptr<osg::Vec3Array> _positionArray;
        osg::ref_ptr<osg::Vec4Array> _colorArray;
        osg::ref_ptr<osg::DrawArrays> _primitive;

        float * h_particleData;
        float h_injectorData[INJT_DATA_MUNB][INJT_DATA_ROWS][INJT_DATA_ROW_ELEM];
        float h_reflectorData[REFL_DATA_MUNB][REFL_DATA_ROWS][REFL_DATA_ROW_ELEM];
        float * h_debugData;
        float old_refl_hits[128];// needs to have same length as d_debugData
        float refl_hits[128];

        double modulator[4];
        double wandPos[3];
        double wandVec[3];
        double wandMat[16];
        size_t sizeDebug;
        int max_age;
        int disappear_age;
        int hand_id;
        int draw_water_sky;
        float state;
        float gravity;
        float colorFreq;
        float alphaControl;
        float anim;
        int trigger,triggerold;
        int but4,but4old;
        int but3,but3old;
        int but2,but2old;
        int but1,but1old;
        int sceneNum;
        int sceneOrder;
        int nextSean;
        int scene0Start;
        int scene1Start;
        int scene2Start;
        int scene3Start;
        int scene4Start;
        int scene0ContextStart;
        int scene1ContextStart;
        int scene2ContextStart;
        int scene3ContextStart;
        int scene4ContextStart;
        int witch_scene;
        int old_witch_scene;
        int sceneChange;

        float showFrameNo;
        float lastShowFrameNo;
	double showStartTime;
	double showTime;
	double lastShowTime;
        double startTime;
        double nowTime;
        double frNum;

        mutable OpenThreads::Mutex _callbackLock;
        mutable std::map<int,bool> _callbackInit;
        mutable std::map<int,CUdeviceptr> d_debugDataMap;
        mutable std::map<int,CUdeviceptr> d_particleDataMap;
};

#endif