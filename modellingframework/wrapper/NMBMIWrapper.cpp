/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2020 Landcare Research New Zealand Ltd
 *
 * This file is part of 'LUMASS', which is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/
/*
 *  NMBMIWrapper.cpp
 *
 *  Created on: 2020-01-13
 *      Author: Alex
 */

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif


#if defined _WIN32
    #define WINCALL __stdcall
    #include <windows.h>
    #include <strsafe.h>
    #include <libloaderapi.h>
    
    #pragma push_macro("GetCurrentTime") 
        #undef GetCurrentTime

        #ifdef LUMASS_PYTHON
        #include "pythonbmi.h"
        #endif

        #include "NMBMIWrapper.h"
    #pragma pop_macro("GetCurrentTime")
#else
    #define WINCALL
    #include <dlfcn.h>
    
    #ifdef LUMASS_PYTHON
    #include "pythonbmi.h"
    #endif

    #include "NMBMIWrapper.h"
#endif

#include <string>
#include <stdlib.h>
#include <yaml-cpp/yaml.h>

#include <QFileInfo>
#include <QDir>

#ifdef LUMASS_PYTHON
#include "Python_wrapper.h"
#endif

#include "itkProcessObject.h"
#include "otbImage.h"

#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"
#include "NMModelController.h"
#include "NMModelComponent.h"

#include "itkProcessObject.h"
#include "otbImage.h"
#include "otbBMIModelFilter.h"


#include "nmbmiwrapper_export.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, class TOutputImage, unsigned int Dimension>
class NMBMIWrapper_Internal

{
public:
    typedef otb::Image<TInputImage, Dimension>  InImgType;
    typedef otb::Image<TOutputImage, Dimension> OutImgType;
    typedef typename otb::BMIModelFilter<InImgType, OutImgType>  FilterType;
    typedef typename FilterType::Pointer        FilterTypePointer;

    // more typedefs
    typedef typename InImgType::PixelType  InImgPixelType;
    typedef typename OutImgType::PixelType OutImgPixelType;

    typedef typename OutImgType::SpacingType      OutSpacingType;
    typedef typename OutImgType::SpacingValueType OutSpacingValueType;
    typedef typename OutImgType::PointType        OutPointType;
    typedef typename OutImgType::PointValueType   OutPointValueType;
    typedef typename OutImgType::SizeValueType    SizeValueType;

    static void createInstance(itk::ProcessObject::Pointer& otbFilter,
                               unsigned int numBands)
    {
        FilterTypePointer f = FilterType::New();
        otbFilter = f;
    }

    static void setNthInput(itk::ProcessObject::Pointer& otbFilter,
                            unsigned int numBands, unsigned int idx, itk::DataObject* dataObj)
    {
        FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
        filter->SetNthInput(idx, dataObj);
    }

    static itk::DataObject* getOutput(itk::ProcessObject::Pointer& otbFilter,
                                      unsigned int numBands, unsigned int idx)
    {
        FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
        return dynamic_cast<OutImgType*>(filter->GetOutput(idx));
    }

    /*$<InternalRATGetSupport>$*/

    /*$<InternalRATSetSupport>$*/


    static void internalLinkParameters(itk::ProcessObject::Pointer& otbFilter,
                                       unsigned int numBands, NMProcess* proc,
                                       unsigned int step, const QMap<QString, NMModelComponent*>& repo)
    {
        NMDebugCtx("NMBMIWrapper_Internal", << "...");

        FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
        NMBMIWrapper* p =
                dynamic_cast<NMBMIWrapper*>(proc);
        // make sure we've got a valid filter object
        if (f == 0)
        {
            NMMfwException e(NMMfwException::NMProcess_UninitialisedProcessObject);
            e.setDescription("We're trying to link, but the filter doesn't seem to be initialised properly!");
            throw e;
            return;
        }

        /* do something reasonable here */
        bool bok;
        int givenStep = step;

        // initialize the BMILibrary
        std::map<std::string, py::object>::iterator pyIt = lumass_python::ctrlPyObjects.find(p->objectName().toStdString());
        if (pyIt != lumass_python::ctrlPyObjects.end())
        {
            bmi::PythonBMI* pybmi = static_cast<bmi::PythonBMI*>(p->mPtrBMILib.get());
            if (pybmi == nullptr)
            {
                std::stringstream sse;
                sse << "PythonBMI - reloading python module for '" << p->objectName().toStdString() << "' failed! "
                    << "I couldn't even get the PythonBMI object!";
                NMMfwException be(NMMfwException::NMProcess_UninitialisedProcessObject);
                be.setDescription(sse.str().c_str());
                throw be;
            }

            std::string pyModuleName = pybmi->getPyModuleName();
            try
            {
                py::module pymod = py::module::import(pyModuleName.c_str());
                if (!pymod.is_none())
                {
                    pymod.reload();
                }

                pyIt->second.dec_ref();
                lumass_python::ctrlPyObjects.erase(pyIt);

                std::string bmiclass = pybmi->getBMIClassName();
                py::object model = pymod.attr(bmiclass.c_str())();
                if (!model.is_none())
                {
                    model.inc_ref();
                    lumass_python::ctrlPyObjects.insert(std::pair<std::string, py::object>(p->objectName().toStdString(), model));
                }
            }
            catch(py::error_already_set& eas)
            {
                std::stringstream sse;
                sse << "PythonBMI - reloading module '" << pyModuleName << "' failed! "
                      << eas.what();
                NMMfwException be(NMMfwException::NMProcess_UninitialisedProcessObject);
                be.setDescription(sse.str().c_str());
                throw be;
            }
            catch(std::exception& se)
            {
                std::stringstream ssestr;
                ssestr << "PythonBMI - reloading module '" << pyModuleName << "' failed! "
                      << se.what();
                NMMfwException see(NMMfwException::NMProcess_UninitialisedProcessObject);
                see.setDescription(ssestr.str().c_str());
                throw see;
            }
        }
        else
        {
            p->initialiseBMILibrary();
        }

        if (p->mPtrBMILib.get() == nullptr)
        {
            NMErr("NMBMIWrapper", << "BMI library initialisation failed!");
            NMMfwException be(NMMfwException::NMProcess_UninitialisedProcessObject);
            be.setDescription("BMI library initialisation failed!");
            throw be;
        }
        f->SetBMIModule(p->mPtrBMILib);

        // pass on the wrapper object name, so the filter can fetch
        // associated python modules from the global module map
        f->SetWrapperName(p->objectName().toStdString());

        f->SetIsStreamable(p->mbIsStreamable);
        f->SetIsThreadable(p->mbIsThreadable);

        QVariant curYamlConfigFileNameVar = p->getParameter("YamlConfigFileName");
        std::string curYamlConfigFileName;
        if (curYamlConfigFileNameVar.isValid())
        {
            curYamlConfigFileName = curYamlConfigFileNameVar.toString().toStdString();
            f->SetYamlConfigFileName(curYamlConfigFileName);
            QString YamlConfigFileNameProvN = QString("nm:YamlConfigFileName=\"%1\"").arg(curYamlConfigFileName.c_str());
            p->addRunTimeParaProvN(YamlConfigFileNameProvN);
        }

        step = p->mapHostIndexToPolicyIndex(givenStep, p->mInputComponents.size());
        std::vector<std::string> userIDs;
        QStringList currentInputs;
        QStringList inputNamesProvVal;
        if (step < p->mInputComponents.size())
        {
            currentInputs = p->mInputComponents.at(step);
            int cnt=0;
            foreach (const QString& input, currentInputs)
            {
                std::stringstream uid;
                uid << "L" << cnt;
                QString inputCompName = p->getModelController()->getComponentNameFromInputSpec(input);
                NMModelComponent* comp = p->getModelController()->getComponent(inputCompName);
                if (comp != 0)
                {
                    if (comp->getUserID().isEmpty())
                    {
                        userIDs.push_back(uid.str());
                    }
                    else
                    {
                        userIDs.push_back(comp->getUserID().toStdString());
                    }
                }
                else
                {
                    userIDs.push_back(uid.str());
                }
                inputNamesProvVal << QString(userIDs.back().c_str());
                ++cnt;
            }
        }
        f->SetInputNames(userIDs);
        QString inputNamesProvN = QString("nm:InputNames=\"%1\"").arg(inputNamesProvVal.join(' '));
        p->addRunTimeParaProvN(inputNamesProvN);


        NMDebugCtx("NMBMIWrapper_Internal", << "done!");
    }
};

InstantiateObjectWrap( NMBMIWrapper, NMBMIWrapper_Internal )
SetNthInputWrap( NMBMIWrapper, NMBMIWrapper_Internal )
GetOutputWrap( NMBMIWrapper, NMBMIWrapper_Internal )
LinkInternalParametersWrap( NMBMIWrapper, NMBMIWrapper_Internal )
/*$<RATGetSupportWrap>$*/
/*$<RATSetSupportWrap>$*/


NMBMIWrapper
::NMBMIWrapper(QObject* parent)
    : mPtrBMILib(nullptr)
{
    this->setParent(parent);
    this->setObjectName("NMBMIWrapper");
    this->mParameterHandling = NMProcess::NM_USE_UP;
}

void NMBMIWrapper::bmilog(int ilevel, const char* msg)
{
    if (this->mLogger == nullptr)
    {
        QString name = this->parent() != nullptr ?
                       this->parent()->objectName() : this->objectName();
        std::cerr << name.toStdString() << " - BMI ERROR: " << msg << std::endl;
        return;
    }

    const int alevel = ilevel;
    switch (alevel)
    {
    case 0:
    case 1: NMLogDebug(<< msg); break;
    case 2: NMLogInfo(<< msg ); break;
    case 3: NMLogWarn(<< msg ); break;
    case 4:
    case 5: NMLogError(<< msg); break;
    default: ;
    }
}

void
NMBMIWrapper::initialiseBMILibrary()
{
    // parse YAML for wrapper relevant settings
    this->parseYamlConfig();

#ifdef LUMASS_PYTHON
    if (this->mBMIComponentType == NM_BMI_COMPONENT_TYPE_PYTHON)
    {
        if (this->parent() != nullptr)
        {
            this->getModelController()->registerPythonRequest(this->parent()->objectName());
        }
    }
#endif


    // ======================================================

    QFileInfo pathInfo(mComponentPath);
    if (!pathInfo.isReadable())
    {
        NMLogError(<< "Couldn't read the BMI model at '"
                   << mComponentPath.toStdString() << "'!");
        return;
    }

    QString baseName = pathInfo.baseName();
    QString path = pathInfo.absolutePath();
    mComponentName = baseName;
    mComponentPath = path;

#ifdef LUMASS_PYTHON
    if (mBMIComponentType == NM_BMI_COMPONENT_TYPE_PYTHON)
    {
        std::string compName = this->parent() != nullptr ? this->parent()->objectName().toStdString()
                                                     : this->objectName().toStdString();
        mPtrBMILib = std::make_shared<bmi::PythonBMI>(bmi::PythonBMI(mComponentName.toStdString(),
                                   mComponentPath.toStdString(),
                                   mBMIClassName.toStdString(),
                                   compName));

        if (this->mLogger != nullptr)
        {
            bmi::PythonBMI* pybmi = static_cast<bmi::PythonBMI*>(mPtrBMILib.get());
            if (pybmi != nullptr)
            {
                pybmi->setWrapLog(this, &NMBMIWrapper::bmilog);
            }
        }

        try
        {
            // remove previous python object, if exists
            {
                std::map<std::string, py::object>::iterator mit = lumass_python::pyObjects->find(compName);
                if (mit != lumass_python::ctrlPyObjects.end())
                {
                    py::object po = mit->second;
                    lumass_python::ctrlPyObjects.erase(mit);
                    po.dec_ref();

                    NMLogInfo(<< "Deleted old module '" << mPtrBMILib->GetComponentName() << "'!");
                }
            }

            mPtrBMILib->Initialize(mYamlConfigFileName.toStdString());
            if (!lumass_python::ctrlPyObjects[compName].is_none())
            {
                lumass_python::pyObjectSinkMap->insert(std::pair<std::string, bool>(compName, mIsSink));
                NMLogInfo(<< "Successfully initialised '" << mPtrBMILib->GetComponentName() << "'!");
            }
            else
            {
                NMLogError(<< "Failed initialising '" << mComponentName.toStdString() << "'!");
            }
        }
        catch(pybind11::cast_error& ce)
        {
            NMLogError(<< "Failed initialisation of '" << mComponentName.toStdString() << "': " << ce.what());
        }
        catch(std::exception& e)
        {
            NMLogError(<< "Failed initialisation of '" << mComponentName.toStdString() << "': " << e.what());
        }
    }
    else
#endif
    {
        if (mBMIComponentType == NM_BMI_COMPONENT_TYPE_NATIVE)
        {
            // init the native BMI library here
            NMLogInfo(<< "Oopsi - can't handle native libs yet :-(")
        }
    }
}

NMBMIWrapper
::~NMBMIWrapper()
{
//    if (mPtrBMILib != nullptr)
//    {
//        delete mPtrBMILib;
//    }
}

void
NMBMIWrapper::parseYamlConfig()
{
    QFileInfo fifo(this->mYamlConfigFileName);
    if (!fifo.isReadable())
    {
        NMLogError(<< "Failed reading BMI model configuration from '"
                   << mYamlConfigFileName.toStdString() << "'!");
        return;
    }


    try
    {
        YAML::Node configFile = YAML::LoadFile(mYamlConfigFileName.toStdString());
        YAML::Node config;

        if (configFile.IsMap() && configFile["LumassBMIConfig"])
        {
            config = configFile["LumassBMIConfig"];
        }
        else
        {
            std::stringstream msg;
            msg << "LumassBMIConfig not found in yaml!";
            NMLogError(<< msg.str().c_str());
            return;
        }

        std::string bmitype;
        if (config.IsMap())
        {
              if (config["type"])
              {
                  bmitype = config["type"].as<std::string>();
              }
              NMLogDebug(<< "type = " << bmitype);

              this->mBMIClassName.clear();
              if (config["name"])
              {
                  mBMIClassName = config["name"].as<std::string>().c_str();
              }
              NMLogDebug(<< "name = " << mBMIClassName.toStdString());

              this->mComponentPath.clear();
              if (config["path"])
              {
                  mComponentPath = config["path"].as<std::string>().c_str();
              }
              NMLogDebug(<< "path = " << mComponentPath.toStdString());

              if (config["issink"])
              {
                  mIsSink = config["issink"].as<bool>();
              }
              NMLogDebug(<< "issink = " << mIsSink);

              if (config["streamable"])
              {
                  mbIsStreamable = config["streamable"].as<bool>();
              }
              NMLogDebug(<< "streamable = " << mbIsStreamable);

              if (config["threadable"])
              {
                  mbIsThreadable = config["threadable"].as<bool>();
              }
              NMLogDebug(<< "threadable = " << mbIsThreadable);
        }

        if (bmitype.compare("bmi-python") == 0)
        {
            this->mBMIComponentType = NM_BMI_COMPONENT_TYPE_PYTHON;
        }
        else
        {
            this->mBMIComponentType = NM_BMI_COMPONENT_TYPE_NATIVE;
        }
    }
    catch(YAML::ParserException& pe)
    {
        NMLogError(<< pe.what());
    }
    catch(YAML::RepresentationException& re)
    {
        NMLogError(<< re.what());
    }
}

void
NMBMIWrapper::setYamlConfigFileName(const QString &YamlConfigFileName)
{
    this->mYamlConfigFileName = YamlConfigFileName;

    emit nmChanged();

    //this->initialiseBMILibrary(YamlConfigFileName);
}