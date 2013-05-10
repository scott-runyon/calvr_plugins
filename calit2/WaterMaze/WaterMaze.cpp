#include "WaterMaze.h"

using namespace cvr;
using namespace osg;
using namespace std;

namespace WaterMaze
{

WaterMaze * WaterMaze::_myPtr = NULL;

CVRPLUGIN(WaterMaze)

WaterMaze::WaterMaze()
{
    _myPtr = this;
    _geoRoot = new osg::MatrixTransform();
    _sppConnected = false;
    _hiddenTile = -1;

    _heightOffset = ConfigManager::getFloat("value", 
        "Plugin.WaterMaze.StartingHeight", 300.0);

//    osg::Matrixd mat;
//    mat.makeTranslate(0, -3000, -_heightOffset);
//   _geoRoot->setMatrix(mat);
    PluginHelper::getObjectsRoot()->addChild(_geoRoot);
    _loaded = false;
}

WaterMaze::~WaterMaze()
{
}

WaterMaze * WaterMaze::instance()
{
    return _myPtr;
}

bool WaterMaze::init()
{
    // Setup menus
    _WaterMazeMenu = new SubMenu("Water Maze");

    PluginHelper::addRootMenuItem(_WaterMazeMenu);

    _loadButton = new MenuButton("Load");
    _loadButton->setCallback(this);
    _WaterMazeMenu->addItem(_loadButton);

    _clearButton = new MenuButton("Clear");
    _clearButton->setCallback(this);
    _WaterMazeMenu->addItem(_clearButton);

    _newTileButton = new MenuButton("New Tile");
    _newTileButton->setCallback(this);
    _WaterMazeMenu->addItem(_newTileButton);

    _gridCB = new MenuCheckbox("Show Grid", false);
    _gridCB->setCallback(this);
    _WaterMazeMenu->addItem(_gridCB);


    _positionMenu = new SubMenu("Reset position");
    _WaterMazeMenu->addItem(_positionMenu);

    for (int i = 0; i < 4; ++i)
    {
        char buffer[50];
        sprintf(buffer, "Corner %d", i + 1);

        MenuButton * button = new MenuButton(buffer);
        button->setCallback(this);
        _positionMenu->addItem(button);

        _positionButtons.push_back(button);
    }

    MenuButton * button = new MenuButton("Center");
    button->setCallback(this);
    _positionMenu->addItem(button);
    _positionButtons.push_back(button);


    // extra output messages
    _debug = (ConfigManager::getEntry("Plugin.WaterMaze.Debug") == "on");

    // Sync random number generator
    if(ComController::instance()->isMaster())
    {
        int seed = time(NULL);
		ComController::instance()->sendSlaves(&seed, sizeof(seed));
        srand(seed);
    } 
    else 
    {
        int seed = 0;
		ComController::instance()->readMaster(&seed, sizeof(seed));
        srand(seed);
    }

    // EEG device communication
    if (ComController::instance()->isMaster())
    {
        int port = 12345;
        init_SPP(port);
    }

    widthTile = ConfigManager::getFloat("value", "Plugin.WaterMaze.WidthTile", 2000.0);
    heightTile = ConfigManager::getFloat("value", "Plugin.WaterMaze.HeightTile", 2000.0);
    numWidth = ConfigManager::getFloat("value", "Plugin.WaterMaze.NumWidth", 10.0);
    numHeight = ConfigManager::getFloat("value", "Plugin.WaterMaze.NumHeight", 10.0);
    depth = ConfigManager::getFloat("value", "Plugin.WaterMaze.Depth", 10.0);
    wallHeight = ConfigManager::getFloat("value", "Plugin.WaterMaze.WallHeight", 2500.0);
    gridWidth = ConfigManager::getFloat("value", "Plugin.WaterMaze.GridWidth", 5.0);

    return true;
}

void WaterMaze::load()
{
    // Set up models

    // Tiles
    osg::Box * box = new osg::Box(osg::Vec3(0,0,0), widthTile, heightTile, depth);
    for (int i = 0; i < numWidth; ++i)
    {
        for (int j = 0; j < numHeight; ++j)
        {
            osg::PositionAttitudeTransform * tilePat = new osg::PositionAttitudeTransform();
            tilePat->setPosition(osg::Vec3((widthTile*i) - (widthTile/2), 
                                           (heightTile*j) - (heightTile/2),
                                            0));
            
            if (i == 0 && j == 0)
            {
                osg::MatrixTransform * tileMat = new osg::MatrixTransform();
                osg::Matrixd mat;
                mat.makeTranslate((tilePat->getPosition() + osg::Vec3(0, -3000, -_heightOffset)));
                tileMat->setMatrix(mat);
                _tilePositions.push_back(tileMat);
            }
            else if (i == 0 && j == numHeight - 1)
            {
                osg::MatrixTransform * tileMat = new osg::MatrixTransform();
                osg::Matrixd mat;
                mat.makeTranslate((tilePat->getPosition()  + osg::Vec3(0, -3000, -_heightOffset)));
                tileMat->setMatrix(mat);
                _tilePositions.push_back(tileMat);
            }
            else if (i == numWidth - 1 && j == 0)
            {
                osg::MatrixTransform * tileMat = new osg::MatrixTransform();
                osg::Matrixd mat;
                mat.makeTranslate((tilePat->getPosition() + osg::Vec3(0, -3000, -_heightOffset)));
                tileMat->setMatrix(mat);
                _tilePositions.push_back(tileMat);
            }
            else if (i == numWidth - 1 && j == numHeight - 1)
            {
                osg::MatrixTransform * tileMat = new osg::MatrixTransform();
                osg::Matrixd mat;
                mat.makeTranslate((tilePat->getPosition() + osg::Vec3(0, -3000, -_heightOffset)));
                tileMat->setMatrix(mat);
                _tilePositions.push_back(tileMat);
            }


            osg::Switch * boxSwitch = new osg::Switch();
            osg::ShapeDrawable * sd = new osg::ShapeDrawable(box);
            sd->setColor(osg::Vec4(1, 1, 1, 1));
            osg::Geode * geode = new osg::Geode();
            geode->addDrawable(sd);
            boxSwitch->addChild(geode);

            sd = new osg::ShapeDrawable(box);
            sd->setColor(osg::Vec4(0, 1, 0, 1));
            geode = new osg::Geode();
            geode->addDrawable(sd);
            boxSwitch->addChild(geode);

            sd = new osg::ShapeDrawable(box);
            sd->setColor(osg::Vec4(1, 0, 0, 1));
            geode = new osg::Geode();
            geode->addDrawable(sd);
            boxSwitch->addChild(geode);

            tilePat->addChild(boxSwitch);
            _geoRoot->addChild(tilePat);

            osg::Vec3 center;
            center = tilePat->getPosition();
            _tileSwitches[center] = boxSwitch;
        }
    }
    
    // Grid
    _gridSwitch = new osg::Switch();
    for (int i = -1; i < numWidth; ++i)
    {
        box = new osg::Box(osg::Vec3(i * widthTile, heightTile * (numHeight-2) * .5, 0), 
            gridWidth, heightTile * numHeight, depth + 1);
        osg::ShapeDrawable * sd = new osg::ShapeDrawable(box);
        sd->setColor(osg::Vec4(0,0,0,1));
        osg::Geode * geode = new osg::Geode();
        geode->addDrawable(sd);
        _gridSwitch->addChild(geode);
    }

    for (int i = -1; i < numHeight; ++i)
    {
        box = new osg::Box(osg::Vec3(widthTile * (numWidth-2) * .5, i * heightTile, 0), 
            widthTile * numWidth, gridWidth, depth + 1);
        osg::ShapeDrawable * sd = new osg::ShapeDrawable(box);
        sd->setColor(osg::Vec4(0,0,0,1));
        osg::Geode * geode = new osg::Geode();
        geode->addDrawable(sd);
        _gridSwitch->addChild(geode);
    }
    _gridSwitch->setAllChildrenOff();
    _geoRoot->addChild(_gridSwitch); 


    // Walls
    osg::ShapeDrawable * sd;
    osg::Geode * geode;
    osg::Vec3 pos;
    
    // far horizontal
    pos = osg::Vec3(widthTile * (numWidth-2) * 0.5, 
                   (numHeight-1) * heightTile , 
                    wallHeight / 2);
    box = new osg::Box(pos, widthTile * numWidth, 4, wallHeight);
    sd = new osg::ShapeDrawable(box);
    sd->setColor(osg::Vec4(1.0, 0.8, 0.8, 1));
    geode = new osg::Geode();
    geode->addDrawable(sd);
    geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    _geoRoot->addChild(geode);
     
    // near horizontal
    pos = osg::Vec3(widthTile * (numWidth-2) * 0.5, 
                    (-1) * heightTile, 
                    wallHeight / 2);
    box = new osg::Box(pos, widthTile * numWidth, 4, wallHeight);
    sd = new osg::ShapeDrawable(box);
    sd->setColor(osg::Vec4(1.0, 1.0, 0.8, 1));
    geode = new osg::Geode();
    geode->addDrawable(sd);
    geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    _geoRoot->addChild(geode);

    // left vertical
    pos = osg::Vec3((numWidth-1) * widthTile, 
                    heightTile * (numHeight-2) * .5, 
                    wallHeight/2);
    box = new osg::Box(pos, 4, heightTile * numHeight, wallHeight);
    sd = new osg::ShapeDrawable(box);
    sd->setColor(osg::Vec4(0.8, 1.0, 0.8, 1));
    geode = new osg::Geode();
    geode->addDrawable(sd);
    geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    _geoRoot->addChild(geode);

    // right vertical
    pos = osg::Vec3((-1) * widthTile, 
                    heightTile * (numHeight-2) * .5, 
                    wallHeight/2);
    box = new osg::Box(pos, 4, heightTile * numHeight, wallHeight);
    sd = new osg::ShapeDrawable(box);
    sd->setColor(osg::Vec4(0.8, 0.8, 1.0, 1));
    geode = new osg::Geode();
    geode->addDrawable(sd);
    geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    _geoRoot->addChild(geode);

    // ceiling
    pos = osg::Vec3((numWidth-2) * widthTile * .5, 
                    (numHeight-2) * heightTile * .5, 
                    wallHeight);
    box = new osg::Box(pos, numWidth * widthTile, numHeight * heightTile, 5);
    sd = new osg::ShapeDrawable(box);
    geode = new osg::Geode();
    geode->addDrawable(sd);
    geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    _geoRoot->addChild(geode);
    
    // floor plane
    pos = osg::Vec3((numWidth-2) * widthTile * .5, 
                    (numHeight-2) * heightTile * .5, 
                    0);
    box = new osg::Box(pos, 200000, 200000, 5);
    sd = new osg::ShapeDrawable(box);
    sd->setColor(osg::Vec4(1, .8, .8, 1));
    geode = new osg::Geode();
    geode->addDrawable(sd);
    _geoRoot->addChild(geode);

    // sky box
    pos = osg::Vec3((numWidth-2) * widthTile * .5, 
                    (numHeight-2) * heightTile * .5, 
                    0);
    box = new osg::Box(pos, 200000, 200000, 200000);
    sd = new osg::ShapeDrawable(box);
    sd->setColor(osg::Vec4(.8, .8, 1, 1));
    geode = new osg::Geode();
    geode->addDrawable(sd);
    _geoRoot->addChild(geode);

    _loaded = true;
}

void WaterMaze::preFrame()
{
    if (_hiddenTile < 0)
    {
        _hiddenTile = rand() % (int)(numWidth * numHeight);
        std::cout << "Hidden tile = " << _hiddenTile << std::endl;
    }

    osg::Vec3 pos = osg::Vec3(0,0,0) * cvr::PluginHelper::getHeadMat() * 
        PluginHelper::getWorldToObjectTransform() * _geoRoot->getInverseMatrix();

    osg::Vec3 bottomLeft, topRight;
    bottomLeft = osg::Vec3(0,0,0) * _geoRoot->getMatrix();
    topRight = osg::Vec3(widthTile * numWidth, heightTile * numHeight, 0) * 
        _geoRoot->getMatrix();

    float xmin, xmax, ymin, ymax;
    xmin = bottomLeft[0];
    ymin = bottomLeft[1];
    xmax = topRight[0];
    ymax = topRight[1];
    
    int i = 0;
    std::map<osg::Vec3, osg::Switch *>::iterator it;
    for (it = _tileSwitches.begin(); it != _tileSwitches.end(); ++it)
    {
        osg::Vec3 center = it->first;
        xmin = center[0] - widthTile/2;
        xmax = center[0] + widthTile/2;
        ymin = center[1] - heightTile/2;
        ymax = center[1] + heightTile/2;
        
        // Occupied tile
        if (pos[0] > xmin && pos[0] < xmax &&
            pos[1] > ymin && pos[1] < ymax)
        {
            //std::cout << "Standing in tile " << i << std::endl;  
            it->second->setSingleChildOn(2);
            if (i == _hiddenTile)
            {
                //_hiddenTile = -1;
                it->second->setSingleChildOn(1);
            }
        }
        // Unoccupied tile
        else
        {
            it->second->setSingleChildOn(0);
        }
        
        // Hidden tile
        if (0)//i == _hiddenTile)
        {
            it->second->setSingleChildOn(1);
        }
        i++;

    //std::cout << "Position: " << pos[0] << " " << pos[1] << " " << pos[2] << std::endl;
    //std::cout << "Min: " << xmin << " " << ymin << std::endl;
    //std::cout << "Max: " << xmax << " " << ymax << std::endl;
    }
    
    //std::cout << "Position: " << pos[0] << " " << pos[1] << " " << pos[2] << std::endl;
    //std::cout << "Min: " << xmin << " " << ymin << std::endl;
    //std::cout << "Max: " << xmax << " " << ymax << std::endl;
}

void WaterMaze::menuCallback(MenuItem * item)
{
    if(item == _loadButton)
    {
        if (!_loaded)
            load();

        PluginHelper::getObjectsRoot()->addChild(_geoRoot);
    }

    else if (item == _clearButton)
    {
        clear();
    }

    else if (item == _newTileButton)
    {
        newHiddenTile();
    }

    else if (item == _gridCB)
    {
        if (_gridCB->getValue())
        {
            _gridSwitch->setAllChildrenOn();
        }
        else
        {
            _gridSwitch->setAllChildrenOff();
        }
    }

    int i = 0;
    for (std::vector<MenuButton*>::iterator it = _positionButtons.begin();
         it != _positionButtons.end(); ++it)
    {
        if ((*it) == item)
        {
            std::cout << (*it)->getText() << std::endl;

/*            osg::Matrixd mat, multMat;

            osg::Vec3 newPos =  _tilePositions[i]->getPosition() * _geoRoot->getInverseMatrix();
            osg::Vec3 origPos = PluginHelper::getObjectMatrix().getTrans();
            osg::Vec3 transVec = newPos - origPos;

            std::cout << "newPos = " << newPos[0] << " " << newPos[1] << " " << newPos[2] << std::endl;
            std::cout << "origPos = " << origPos[0] << " " << origPos[1] << " " << origPos[2] << std::endl;
            std::cout << "transVec = " << transVec[0] << " " << transVec[1] << " " << transVec[2] << std::endl;

            mat = PluginHelper::getObjectMatrix();
            multMat.makeTranslate(transVec[0], transVec[1], 0);

            //mat.setTrans(multMat * );
            mat = multMat * mat; */
            
            osg::Matrixd mat;
            mat = _tilePositions[i]->getMatrix();
            PluginHelper::setObjectMatrix(mat);
        }
        ++i;
    }
}

bool WaterMaze::processEvent(InteractionEvent * event)
{
    return false;

    TrackedButtonInteractionEvent * tie = event->asTrackedButtonEvent();

    if (tie)
    {
        if(tie->getHand() == 0 && tie->getButton() == 0)
        {
            if (tie->getInteraction() == BUTTON_DOWN)
            {
                return true;
            }
            else if (tie->getInteraction() == BUTTON_DRAG)
            {
                return true;
            }
            else if (tie->getInteraction() == BUTTON_UP)
            {
                return true;
            }
            return true;
        }
    }
    return true;
}

void WaterMaze::clear()
{
    PluginHelper::getObjectsRoot()->removeChild(_geoRoot);
    _loaded = false;
}

void WaterMaze::reset()
{

}

void WaterMaze::newHiddenTile()
{
    _hiddenTile = -1;
}

};
