/***************************************************************
* File Name: DSVirtualSphere.cpp
*
* Description: 
*
* Written by ZHANG Lelin on Oct 5, 2010
*
***************************************************************/
#include "DSVirtualSphere.h"

using namespace std;
using namespace osg;

//Constructor
DSVirtualSphere::DSVirtualSphere()
{
    CAVEAnimationModeler::ANIMCreateVirtualSphere(&mPATransFwd, &mPATransBwd);
    this->addChild(mPATransFwd);	//  child #0
    this->addChild(mPATransBwd);	//  child #1

    fwdVec.push_back(mPATransFwd);
    bwdVec.push_back(mPATransBwd);

    setSingleChildOn(0);

    mDSIntersector = new DSIntersector();
    mDOIntersector = new DOIntersector();
    mDSIntersector->loadRootTargetNode(NULL, NULL);
    mDOIntersector->loadRootTargetNode(NULL, NULL);
    mDSIntersector->loadRootTargetNode(gDesignStateRootGroup, mPATransFwd->getChild(0));
    mIsOpen = false;
    mDevPressedFlag = false;
}


void DSVirtualSphere::addChildState(DesignStateBase* ds)
{
    if (!ds)
        return;

    mChildStates.push_back(ds);

    float z = mChildStates.size();
    z = -z + 0.5;
    
    osg::Vec3 startPos(-1.5, 0, 0);
    osg::Vec3 pos(-1.5, 0, z);
    AnimationPath* animationPathScaleFwd = new AnimationPath();
    AnimationPath* animationPathScaleBwd = new AnimationPath();
    animationPathScaleFwd->setLoopMode(AnimationPath::NO_LOOPING);
    animationPathScaleBwd->setLoopMode(AnimationPath::NO_LOOPING);

    Vec3 scaleFwd, scaleBwd;
    float step = 1.f / ANIM_VIRTUAL_SPHERE_NUM_SAMPS;
    for (int j = 0; j < ANIM_VIRTUAL_SPHERE_NUM_SAMPS + 1; j++)
    {
        float val = j * step;
        scaleFwd = Vec3(val, val, val);
        scaleBwd = Vec3(1-val, 1-val, 1-val);

        osg::Vec3 diff = startPos - pos;
        osg::Vec3 fwd, bwd;

        for (int i = 0; i < 3; ++i)
            diff[i] *= val;

        fwd = startPos - diff;
        bwd = pos + diff;

        animationPathScaleFwd->insert(val, AnimationPath::ControlPoint(fwd, Quat(), scaleFwd));
        animationPathScaleBwd->insert(val, AnimationPath::ControlPoint(bwd, Quat(), scaleBwd));
    }

    AnimationPathCallback *animCallbackFwd = new AnimationPathCallback(animationPathScaleFwd, 
                        0.0, 1.f / ANIM_VIRTUAL_SPHERE_LAPSE_TIME);
    AnimationPathCallback *animCallbackBwd = new AnimationPathCallback(animationPathScaleBwd, 
                        0.0, 1.f / ANIM_VIRTUAL_SPHERE_LAPSE_TIME); 

    osg::PositionAttitudeTransform *fwd, *bwd;
    fwd = new osg::PositionAttitudeTransform();
    bwd = new osg::PositionAttitudeTransform();

    fwd->setUpdateCallback(animCallbackFwd);
    bwd->setUpdateCallback(animCallbackBwd);

    fwd->addChild(ds);
    bwd->addChild(ds);

    fwdVec.push_back(fwd);
    bwdVec.push_back(bwd);

    this->addChild(fwd);
    this->addChild(bwd);

    setAllChildrenOn();
    mActiveSubState = NULL;
}


/***************************************************************
* Function: setObjectEnabled()
*
* Description:
*
***************************************************************/
void DSVirtualSphere::setObjectEnabled(bool flag)
{
    mObjEnabledFlag = flag;

    if (!mPATransFwd || !mPATransBwd) 
        return;

    AnimationPathCallback* animCallback = NULL;
    if (flag)
    {
        if (!mIsOpen) // open menu
        {
           // setSingleChildOn(0);
           // animCallback = dynamic_cast <AnimationPathCallback*> (mPATransFwd->getUpdateCallback());
           // mDSIntersector->loadRootTargetNode(gDesignStateRootGroup, mPATransFwd);
           // mDOIntersector->loadRootTargetNode(gDesignObjectRootGroup, NULL);
            
            setSingleChildOn(0);
            for (int i = 0; i < fwdVec.size(); ++i)
            {
                setChildValue(fwdVec[i], true);
                animCallback = dynamic_cast <AnimationPathCallback*> (fwdVec[i]->getUpdateCallback());

                if (animCallback)
                    animCallback->reset();
            }

            std::list<DesignStateBase*>::iterator it;
            for (it = mChildStates.begin(); it != mChildStates.end(); ++it)
            {
                (*it)->setObjectEnabled(true);
            }
            mIsOpen = true;
            mDSIntersector->loadRootTargetNode(gDesignStateRootGroup, mPATransFwd);
        }
        else // close menu
        {
            setSingleChildOn(0);
            for (int i = 1; i < bwdVec.size(); ++i)
            {
                setChildValue(bwdVec[i], true);
                animCallback = dynamic_cast <AnimationPathCallback*> (bwdVec[i]->getUpdateCallback());

                if (animCallback)
                    animCallback->reset();
            }

            std::list<DesignStateBase*>::iterator it;
            for (it = mChildStates.begin(); it != mChildStates.end(); ++it)
            {
                (*it)->setObjectEnabled(false);
            }
            mIsOpen = false;
            mDSIntersector->loadRootTargetNode(gDesignStateRootGroup, mPATransFwd->getChild(0));
        }
    } 
    else  // close menu
    {
        setSingleChildOn(0);
        for (int i = 1; i < bwdVec.size(); ++i)
        {
            setChildValue(bwdVec[i], true);
            animCallback = dynamic_cast <AnimationPathCallback*> (bwdVec[i]->getUpdateCallback());

            if (animCallback)
                animCallback->reset();
        }

        std::list<DesignStateBase*>::iterator it;
        for (it = mChildStates.begin(); it != mChildStates.end(); ++it)
        {
            (*it)->setObjectEnabled(false);
        }
        mIsOpen = false;
        mDSIntersector->loadRootTargetNode(gDesignStateRootGroup, mPATransFwd->getChild(0));
    }

    if (animCallback) 
        animCallback->reset();
    
    mActiveSubState = NULL;
}


/***************************************************************
* Function: switchToPrevState()
***************************************************************/
void DSVirtualSphere::switchToPrevSubState()
{
    if (mActiveSubState)
    {
        mActiveSubState->switchToPrevSubState();
        return;
    }

/*    AnimationPathCallback* animCallback = NULL;
    setAllChildrenOff();
    if (mIsOpen)
    {
        setSingleChildOn(0);
        for (int i = 1; i < bwdVec.size(); ++i)
        {
            setChildValue(bwdVec[i], true);
            animCallback = dynamic_cast <AnimationPathCallback*> (bwdVec[i]->getUpdateCallback());
            mDSIntersector->loadRootTargetNode(gDesignStateRootGroup, bwdVec[i]);

            if (animCallback)
                animCallback->reset();
        }
        mDSIntersector->loadRootTargetNode(gDesignStateRootGroup, mPATransFwd);

        std::list<DesignStateBase*>::iterator it;
        for (it = mChildStates.begin(); it != mChildStates.end(); ++it)
        {
            (*it)->setObjectEnabled(true);
        }
        mIsOpen = false;
    }
    else
    {
        setSingleChildOn(0);
        for (int i = 1; i < fwdVec.size(); ++i)
        {
            setChildValue(fwdVec[i], true);
            animCallback = dynamic_cast <AnimationPathCallback*> (fwdVec[i]->getUpdateCallback());
            mDSIntersector->loadRootTargetNode(gDesignStateRootGroup, fwdVec[i]);

            if (animCallback)
                animCallback->reset();
        }

        std::list<DesignStateBase*>::iterator it;
        for (it = mChildStates.begin(); it != mChildStates.end(); ++it)
        {
            (*it)->setObjectEnabled(true);
        }

        mDSIntersector->loadRootTargetNode(gDesignStateRootGroup, mPATransFwd);
        mIsOpen = true;
    }
    */
}


/***************************************************************
* Function: switchToNextState()
***************************************************************/
void DSVirtualSphere::switchToNextSubState()
{
    if (mActiveSubState)
    {
        mActiveSubState->switchToNextSubState();
        return;
    }

    switchToPrevSubState();
}


/***************************************************************
* Function: inputDevPressEvent()
***************************************************************/
bool DSVirtualSphere::inputDevPressEvent(const osg::Vec3 &pointerOrg, const osg::Vec3 &pointerPos)
{
    mDevPressedFlag = true;
    std::list<DesignStateBase*>::iterator it;
    it = mChildStates.begin();

    for (int i = 0; i < fwdVec.size(); ++i)
    {
        mDSIntersector->loadRootTargetNode(gDesignStateRootGroup, fwdVec[i]->getChild(0));
        //setChildValue(fwdVec[i], true);
        if (mDSIntersector->test(pointerOrg, pointerPos))
        {
            // open/close menu
            if (i == 0)
            {
                switchToPrevSubState();
            }
            // pass on event
            else
            {
                //std::cout << "passing on event " << i << std::endl;
                (*it)->inputDevPressEvent(pointerOrg, pointerPos);
            }
        }
        it++;
    }
    
    for (it = mChildStates.begin(); it != mChildStates.end(); ++it)
    {
        if ((*it)->test(pointerOrg, pointerPos))
        {
            mActiveSubState = (*it);
            mDSIntersector->loadRootTargetNode(gDesignStateRootGroup, mPATransFwd->getChild(0));
            return (*it)->inputDevPressEvent(pointerOrg, pointerPos);
        }
    }
    mActiveSubState = NULL;

    mDSIntersector->loadRootTargetNode(gDesignStateRootGroup, mPATransFwd->getChild(0));
    return false;
}


/***************************************************************
* Function: inputDevMoveEvent()
***************************************************************/
void DSVirtualSphere::inputDevMoveEvent(const osg::Vec3 &pointerOrg, const osg::Vec3 &pointerPos)
{
    if (mActiveSubState)
        mActiveSubState->inputDevMoveEvent(pointerOrg, pointerPos);
}


/***************************************************************
* Function: inputDevReleaseEvent()
***************************************************************/
bool DSVirtualSphere::inputDevReleaseEvent()
{
    mDevPressedFlag = false;
    if (mActiveSubState)
        return mActiveSubState->inputDevReleaseEvent();

    return false;
}


/***************************************************************
* Function: update()
***************************************************************/
void DSVirtualSphere::update()
{
    if (mActiveSubState)
        mActiveSubState->update();
}


/***************************************************************
* Function: setHighlight()
***************************************************************/
void DSVirtualSphere::setHighlight(bool isHighlighted, const osg::Vec3 &pointerOrg, const osg::Vec3 &pointerPos) 
{
//    std::cout << "virtual sphere setHighlight " << isHighlighted << std::endl;
    if (isHighlighted)
    {
        for (int i = 0; i < fwdVec.size(); ++i)
        {
            mDSIntersector->loadRootTargetNode(gDesignStateRootGroup, fwdVec[i]->getChild(0));

            if (mDSIntersector->test(pointerOrg, pointerPos))
            {

                osg::Sphere *sphere = new osg::Sphere();
                osg::ShapeDrawable *sd = new osg::ShapeDrawable(sphere);
                mHighlightGeode = new osg::Geode();
                sphere->setRadius(0.25);
                sd->setColor(osg::Vec4(1,1,1,0.5));
                mHighlightGeode->addDrawable(sd);
//                fwdVec[i]->addChild(mHighlightGeode);

                StateSet *stateset = sd->getOrCreateStateSet();
                stateset->setMode(GL_BLEND, StateAttribute::OVERRIDE | StateAttribute::ON);
                stateset->setMode(GL_CULL_FACE, StateAttribute::OVERRIDE | StateAttribute::ON);
                stateset->setRenderingHint(StateSet::TRANSPARENT_BIN);

            }
        }
    }
    else
    {
        for (int i = 0; i < fwdVec.size(); ++i)
        {
            fwdVec[i]->removeChild(mHighlightGeode);
        }
    }
    mDSIntersector->loadRootTargetNode(gDesignStateRootGroup, mPATransFwd->getChild(0));
}

