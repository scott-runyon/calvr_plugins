#include "FlowVis.h"
#include "FlowObject.h"

#include <cvrConfig/ConfigManager.h>
#include <cvrKernel/PluginHelper.h>

#include <osg/PrimitiveSet>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PolygonMode>
#include <osg/CullFace>

#include <algorithm>
#include <iostream>
#include <cstring>
#include <fstream>
#include <cstdio>
#include <climits>
#include <cmath>

using namespace cvr;

CVRPLUGIN(FlowVis)

FlowVis::FlowVis()
{
    _loadedSet = NULL;
    _loadedObject = NULL;
}

FlowVis::~FlowVis()
{
}

bool FlowVis::init()
{
    _flowMenu = new SubMenu("FlowVis");
    PluginHelper::addRootMenuItem(_flowMenu);

    _loadMenu = new SubMenu("Load");
    _flowMenu->addItem(_loadMenu);

    std::vector<std::string> tags;
    ConfigManager::getChildren("Plugin.FlowVis.Files",tags);

    for(int i = 0; i < tags.size(); ++i)
    {
	MenuButton * button = new MenuButton(tags[i]);
	button->setCallback(this);
	_loadMenu->addItem(button);
	_loadButtons.push_back(button);

	FileInfo * fi = new FileInfo;
	fi->path = ConfigManager::getEntry("path",std::string("Plugin.FlowVis.Files.") + tags[i],"");
	fi->start = ConfigManager::getInt("start",std::string("Plugin.FlowVis.Files.") + tags[i],0);
	fi->frames = ConfigManager::getInt("frames",std::string("Plugin.FlowVis.Files.") + tags[i],0);
	_loadFiles.push_back(fi);
    }

    _removeButton = new MenuButton("Remove");
    _removeButton->setCallback(this);
    _flowMenu->addItem(_removeButton);

    return true;
}

void FlowVis::preFrame()
{
    if(_loadedObject)
    {
	_loadedObject->perFrame();
    }
}

void FlowVis::menuCallback(MenuItem * item)
{
    if(item == _removeButton)
    {
	if(_loadedSet)
	{
	    delete _loadedObject;
	    _loadedObject = NULL;
	    if(_loadedSet->type == FDT_VTK)
	    {
		for(int i = 0; i < _loadedSet->frameList.size(); ++i)
		{
		    deleteVTKFrame(_loadedSet->frameList[i]);
		}
		delete _loadedSet;
		_loadedSet = NULL;
	    }
	}
	return;
    }

    for(int i = 0; i < _loadButtons.size(); ++i)
    {
	if(item == _loadButtons[i])
	{
	    if(_loadedSet)
	    {
		delete _loadedObject;
		_loadedObject = NULL;
		if(_loadedSet->type == FDT_VTK)
		{
		    for(int i = 0; i < _loadedSet->frameList.size(); ++i)
		    {
			deleteVTKFrame(_loadedSet->frameList[i]);
		    }
		    delete _loadedSet;
		    _loadedSet = NULL;
		}
	    }

	    size_t pos;
	    pos = _loadFiles[i]->path.find_last_of('.');

	    if(pos != std::string::npos)
	    {
		std::string ext = _loadFiles[i]->path.substr(pos,_loadFiles[i]->path.length()-pos);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		if(ext == ".vtk")
		{
		    FlowDataSet * set;
		    if(!(set = parseVTK(_loadFiles[i]->path, _loadFiles[i]->start, _loadFiles[i]->frames)))
		    {
			std::cerr << "Error parsing vtk files path: " << _loadFiles[i]->path << " start: " << _loadFiles[i]->start << " frames: " << _loadFiles[i]->frames << std::endl;
			return;
		    }
		    else
		    {
			std::cerr << "Parsed VTK file(s)." << std::endl;

			if(set->frameList.size())
			{
			    _loadedSet = set;
			    _loadedObject = new FlowObject(set,_loadFiles[i]->path,true,false,false,true,true);
			    _loadedObject->setBoundsCalcMode(SceneObject::MANUAL);
			    _loadedObject->setBoundingBox(set->frameList[0]->bb);

			    PluginHelper::registerSceneObject(_loadedObject,"FlowVis");
			    _loadedObject->attachToScene();
			}
		    }
		}
		else
		{
		    std::cerr << "Unknown file extension: " << ext << std::endl;
		    return;
		}
	    }
	    else
	    {
		std::cerr << "No file extension for path: " << _loadFiles[i]->path << std::endl;
		return;
	    }

	    return;
	}
    }
}

FlowDataSet * FlowVis::parseVTK(std::string filePath, int start, int frames)
{
    if(frames <= 0)
    {
	return NULL;
    }

    bool status = true;

    FlowDataSet * dataSet = new FlowDataSet;
    dataSet->info.path = filePath;
    dataSet->info.start = start;
    dataSet->info.frames = frames;
    dataSet->type = FDT_VTK;

    // change this to all frames after testing
    for(int i = 0; i < frames; ++i)
    {
	char file[1024];
	snprintf(file,1023,filePath.c_str(),start+i);
	std::cerr << "Loading file: " << file << std::endl;

	char buffer[1024];
	FILE * fileptr = fopen(file,"r");

	int points;
	int numIndices;

	VTKDataFrame * frame = new VTKDataFrame;

	if(fileptr)
	{
	    while(!feof(fileptr))
	    {
		int count = 0;
		int ret = fscanf(fileptr,"#%n",&count);
		
		while(!feof(fileptr) && count)
		{
		    //std::cerr << "got comment" << std::endl;
		    char c = '\0';
		    while(!feof(fileptr) && c != '\n')
		    {
			fread(&c,sizeof(char),1,fileptr);
		    }
		    count = 0;
		    ret = fscanf(fileptr,"#%n",&count);
		}

		int read = fscanf(fileptr,"%s",buffer);
		if(read <= 0)
		{
		    break;
		}

		if(!strcmp(buffer,"LS-DYNA"))
		{
		    //std::cerr << "LS-DYNA" << std::endl;
		    fscanf(fileptr,"%*s %*s");
		}
		else if(!strcmp(buffer,"ASCII"))
		{
		    //std::cerr << "ASCII" << std::endl;
		}
		else if(!strcmp(buffer,"DATASET"))
		{
		    char type[1024];
		    fscanf(fileptr,"%s",type);
		    if(strcmp(type,"UNSTRUCTURED_GRID"))
		    {
			std::cerr << "Unknown dataset type: " << type << std::endl;
			status = false;
			deleteVTKFrame(frame);
			break;
		    }
		}
		else if(!strcmp(buffer,"POINTS"))
		{
		    fscanf(fileptr,"%d %*s",&points);
		    //std::cerr << "Points: " << points << std::endl;
		    frame->verts = new osg::Vec3Array(points);
		    for(int i = 0; i < points; ++i)
		    {
			fscanf(fileptr,"%f %f %f",&frame->verts->at(i).x(),&frame->verts->at(i).y(),&frame->verts->at(i).z());
			frame->bb.expandBy(frame->verts->at(i));
		    }
		    std::cerr << "Bounds: min x: " << frame->bb.xMin() << " y: " << frame->bb.yMin() << " z: " << frame->bb.zMin() << std::endl;
		    std::cerr << "Bounds: max x: " << frame->bb.xMax() << " y: " << frame->bb.yMax() << " z: " << frame->bb.zMax() << std::endl;
		}
		else if(!strcmp(buffer,"CELLS"))
		{
		    int dataCount = 0;
		    fscanf(fileptr,"%d %d",&numIndices,&dataCount);
		    frame->indices = new osg::DrawElementsUInt(osg::PrimitiveSet::QUADS,numIndices*4);
		    frame->indices->resize(numIndices*4);

		    for(int i = 0; i < numIndices; ++i)
		    {
			int icount = 0;
			fscanf(fileptr,"%d",&icount);
			if(icount != 4)
			{
			    std::cerr << "Non quad found." << std::endl;
			    break;
			}
			int i1,i2,i3,i4;
			fscanf(fileptr,"%d %d %d %d",&i1, &i2, &i3, &i4);
			frame->indices->at((i*4)+0) = i1;
			frame->indices->at((i*4)+1) = i2;
			frame->indices->at((i*4)+2) = i3;
			frame->indices->at((i*4)+3) = i4;
		    }

		    numIndices *= 4;
		}
		else if(!strcmp(buffer,"CELL_TYPES"))
		{
		    int dataCount = 0;
		    fscanf(fileptr,"%d",&dataCount);
		    frame->cellTypes = new osg::IntArray(dataCount);
		    for(int i = 0; i < dataCount; ++i)
		    {
			fscanf(fileptr,"%d",&frame->cellTypes->at(i));
		    }
		}
		else if(!strcmp(buffer,"CELL_DATA"))
		{
		    int dataCount = 0;
		    fscanf(fileptr,"%d",&dataCount);
		    while(!feof(fileptr))
		    {
			long int fpos = ftell(fileptr);
			char dataType[1024];
			fscanf(fileptr,"%s",dataType);
			if(!strcmp(dataType,"SCALARS") || !strcmp(dataType,"VECTORS"))
			{
			    VTKDataAttrib * attrib = parseVTKAttrib(fileptr,dataType,dataCount);
			    if(attrib)
			    {
				frame->cellData.push_back(attrib);
			    }
			    else
			    {
				status = false;
				break;
			    }
			}
			else
			{
			    fseek(fileptr,fpos,SEEK_SET);
			    break;
			}
		    }
		    if(!status)
		    {
			deleteVTKFrame(frame);
			break;
		    }
		}
		else if(!strcmp(buffer,"POINT_DATA"))
		{
		    int dataCount = 0;
		    fscanf(fileptr,"%d",&dataCount);
		    while(!feof(fileptr))
		    {
			long int fpos = ftell(fileptr);
			char dataType[1024];
			fscanf(fileptr,"%s",dataType);
			if(feof(fileptr))
			{
			    break;
			}
			if(!strcmp(dataType,"SCALARS") || !strcmp(dataType,"VECTORS"))
			{
			    VTKDataAttrib * attrib = parseVTKAttrib(fileptr,dataType,dataCount);
			    if(attrib)
			    {
				frame->pointData.push_back(attrib);
			    }
			    else
			    {
				status = false;
				break;
			    }
			}
			else
			{
			    fseek(fileptr,fpos,SEEK_SET);
			    break;
			}
		    }
		    if(!status)
		    {
			deleteVTKFrame(frame);
			break;
		    }
		}
		else
		{
		    std::cerr << "Unknown section: " << buffer << std::endl;
		    status = false;
		    deleteVTKFrame(frame);
		    break;
		}
	    }
	    fclose(fileptr);

	    extractSurfaceVTK(frame);
	    dataSet->frameList.push_back(frame);
	}
	else
	{
	    std::cerr << "Unable to open file." << std::endl;
	    status = false;
	    deleteVTKFrame(frame);
	    break;
	}
    }

    if(status)
    {
	for(int i = 0; i < dataSet->frameList.size(); ++i)
	{
	    for(int j = 0; j < dataSet->frameList[i]->pointData.size(); ++j)
	    {
		if(dataSet->frameList[i]->pointData[j]->dataType == VDT_DOUBLE)
		{
		    std::map<std::string,std::pair<float,float> >::iterator it = dataSet->attribRanges.find(dataSet->frameList[i]->pointData[j]->name);
		    if(it == dataSet->attribRanges.end())
		    {
			dataSet->attribRanges[dataSet->frameList[i]->pointData[j]->name] = std::pair<float,float>(FLT_MAX,FLT_MIN);
			it = dataSet->attribRanges.find(dataSet->frameList[i]->pointData[j]->name);
		    }

		    if(dataSet->frameList[i]->pointData[j]->floatMin < it->second.first)
		    {
			it->second.first = dataSet->frameList[i]->pointData[j]->floatMin;
		    }
		    if(dataSet->frameList[i]->pointData[j]->floatMax > it->second.second)
		    {
			it->second.second = dataSet->frameList[i]->pointData[j]->floatMax;
		    }
		}
	    }
	}

	return dataSet;
    }
    else
    {
	for(int i = 0; i < dataSet->frameList.size(); ++i)
	{
	    deleteVTKFrame(dataSet->frameList[i]);
	}
	delete dataSet;
	return NULL;
    }
}

VTKDataAttrib * FlowVis::parseVTKAttrib(FILE * file, std::string type, int count)
{
    VTKDataAttrib * attrib = new VTKDataAttrib;

    if(type == "SCALARS")
    {
	attrib->attribType = VAT_SCALARS;
	char name[1024];
	char valType[1024];
	fscanf(file,"%s %s",name,valType);
	attrib->name = name;
	
	std::string svalType(valType);

	// clear lookup table entry
	fscanf(file,"%*s %*s");

	if(svalType == "int")
	{
	    attrib->dataType = VDT_INT;
	    attrib->intData = new osg::IntArray(count);
	    attrib->intMin = INT_MAX;
	    attrib->intMax = INT_MIN;
	    for(int i = 0; i < count; ++i)
	    {
		fscanf(file,"%d",&attrib->intData->at(i));
		if(attrib->intData->at(i) < attrib->intMin)
		{
		    attrib->intMin = attrib->intData->at(i);
		}
		if(attrib->intData->at(i) > attrib->intMax)
		{
		    attrib->intMax = attrib->intData->at(i);
		}
		//std::cerr << "attrib: " << attrib->intData->at(i) << std::endl;
		/*if(attrib->intData->at(i) != -1)
		{
		    attrib->intData->at(i) = 8;
		}*/
	    }
	    //std::cerr << "Int name: " << attrib->name <<  " min: " << attrib->intMin << " max: " << attrib->intMax << std::endl;
	}
	else if(svalType == "double")
	{
	    attrib->dataType = VDT_DOUBLE;
	    attrib->floatData = new osg::FloatArray(count);
	    attrib->floatMin = FLT_MAX;
	    attrib->floatMax = FLT_MIN;
	    for(int i = 0; i < count; ++i)
	    {
		fscanf(file,"%f",&attrib->floatData->at(i));
		if(attrib->floatData->at(i) < attrib->floatMin)
		{
		    attrib->floatMin = attrib->floatData->at(i);
		}
		if(attrib->floatData->at(i) > attrib->floatMax)
		{
		    attrib->floatMax = attrib->floatData->at(i);
		}
	    }
	}
	else
	{
	    std::cerr << "Unknown attribute value type: " << svalType << std::endl;
	    delete attrib;
	    return NULL;
	}
    }
    else if(type == "VECTORS")
    {
	attrib->attribType = VAT_VECTORS;
	char name[1024];
	char valType[1024];
	fscanf(file,"%s %s",name,valType);
	attrib->name = name;
	
	std::string svalType(valType);
	if(svalType == "double")
	{
	    attrib->dataType = VDT_DOUBLE;
	    attrib->floatMin = FLT_MAX;
	    attrib->floatMax = FLT_MIN;
	    attrib->vecData = new osg::Vec3Array(count);
	    for(int i = 0; i < count; ++i)
	    {
		fscanf(file,"%f %f %f",&attrib->vecData->at(i).x(),&attrib->vecData->at(i).y(),&attrib->vecData->at(i).z());
		float mag = attrib->vecData->at(i).length();
		if(mag > attrib->floatMax)
		{
		    attrib->floatMax = mag;
		}
		if(mag < attrib->floatMin)
		{
		    attrib->floatMin = mag;
		}
	    }
	}
	else
	{
	    std::cerr << "Unknown vector attribute value type: " << svalType << std::endl;
	    delete attrib;
	    return NULL;
	}
    }
    else
    {
	std::cerr << "Error: Unknown Attribute type: " << type << std::endl;
	delete attrib;
	return NULL;
    }

    return attrib;
}

struct face
{
    face()
    {
	indices[0] = indices[1] = indices[2] = 0.0;
    }
    face(unsigned int a, unsigned int b, unsigned int c)
    {
	indices[0] = a;
	indices[1] = b;
	indices[2] = c;
    }
    unsigned int indices[3];
};

void FlowVis::extractSurfaceVTK(VTKDataFrame * frame)
{
    std::cerr << "extracting surface" << std::endl;
    frame->surfaceInd = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES,0);

    std::map<unsigned int, std::map<unsigned int, std::map<unsigned int, std::pair<face,int> > > > faces;
    for(int i = 0; i < frame->indices->size(); i += 4)
    {
	std::vector<face> faceList;
	faceList.push_back(face(frame->indices->at(i+0),frame->indices->at(i+1),frame->indices->at(i+3)));
	faceList.push_back(face(frame->indices->at(i+0),frame->indices->at(i+2),frame->indices->at(i+1)));
	faceList.push_back(face(frame->indices->at(i+0),frame->indices->at(i+3),frame->indices->at(i+2)));
	faceList.push_back(face(frame->indices->at(i+1),frame->indices->at(i+2),frame->indices->at(i+3)));

	for(int j = 0; j < faceList.size(); ++j)
	{
	    unsigned int first,second,third;
	    if(faceList[j].indices[0] >= faceList[j].indices[1])
	    {
		if(faceList[j].indices[0] >= faceList[j].indices[2])
		{
		    first = faceList[j].indices[0];
		    if(faceList[j].indices[1] >= faceList[j].indices[2])
		    {
			second = faceList[j].indices[1];
			third = faceList[j].indices[2];
		    }
		    else
		    {
			second = faceList[j].indices[2];
			third = faceList[j].indices[1];
		    }
		}
		else
		{
		    first = faceList[j].indices[2];
		    second = faceList[j].indices[0];
		    third = faceList[j].indices[1];
		}
	    }
	    else if(faceList[j].indices[0] >= faceList[j].indices[2])
	    {
		first = faceList[j].indices[1];
		second = faceList[j].indices[0];
		third = faceList[j].indices[2];
	    }
	    else
	    {
		third = faceList[j].indices[0];
		if(faceList[j].indices[1] >= faceList[j].indices[2])
		{
		    first = faceList[j].indices[1];
		    second = faceList[j].indices[2];
		}
		else
		{
		    first = faceList[j].indices[2];
		    second = faceList[j].indices[1];
		}
	    }

	    faces[first][second][third].first = faceList[j];
	    faces[first][second][third].second++;
	}
    }

    for(std::map<unsigned int, std::map<unsigned int, std::map<unsigned int, std::pair<face,int> > > >::iterator it = faces.begin(); it != faces.end(); ++it)
    {
	for(std::map<unsigned int, std::map<unsigned int, std::pair<face,int> > >::iterator itt = it->second.begin(); itt != it->second.end(); ++itt)
	{
	    for(std::map<unsigned int, std::pair<face,int> >::iterator ittt = itt->second.begin(); ittt != itt->second.end(); ++ittt)
	    {
		if(ittt->second.second == 1)
		{
		    frame->surfaceInd->push_back(ittt->second.first.indices[0]);
		    frame->surfaceInd->push_back(ittt->second.first.indices[1]);
		    frame->surfaceInd->push_back(ittt->second.first.indices[2]);
		}
	    }
	}
    }

    std::cerr << "done: got " << frame->surfaceInd->size() << " indices." << std::endl;
}

void FlowVis::deleteVTKFrame(VTKDataFrame * frame)
{
    if(!frame)
    {
	return;
    }

    for(int j = 0; j < frame->cellData.size(); ++j)
    {
	delete frame->cellData[j];
    }

    for(int j = 0; j < frame->pointData.size(); ++j)
    {
	delete frame->pointData[j];
    }

    delete frame;
}
