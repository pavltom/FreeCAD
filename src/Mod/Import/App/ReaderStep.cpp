// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
#include <Standard_Version.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <Transfer_TransientProcess.hxx>
#include <XSControl_TransferReader.hxx>
#include <XSControl_WorkSession.hxx>
#endif

#include "ReaderStep.h"
#include <Base/Exception.h>
#include <Mod/Part/App/encodeFilename.h>

using namespace Import;

ReaderStep::ReaderStep(const Base::FileInfo& file)  // NOLINT
    : file {file}
{
#if OCC_VERSION_HEX >= 0x070800
    codePage = Resource_FormatType_UTF8;
#endif
}

void ReaderStep::read(Handle(TDocStd_Document) hDoc)  // NOLINT
{
    std::string utf8Name = file.filePath();
    std::string name8bit = Part::encodeFilename(utf8Name);
    STEPCAFControl_Reader aReader;
    aReader.SetColorMode(true);
    aReader.SetNameMode(true);
    aReader.SetLayerMode(true);
    aReader.SetSHUOMode(true);
#if OCC_VERSION_HEX < 0x070800
    if (aReader.ReadFile(name8bit.c_str()) != IFSelect_RetDone) {
#else
    Handle(StepData_StepModel) aStepModel = new StepData_StepModel;
    aStepModel->InternalParameters.InitFromStatic();
    aStepModel->SetSourceCodePage(codePage);
    if (aReader.ReadFile(name8bit.c_str(), aStepModel->InternalParameters) != IFSelect_RetDone) {
#endif
        throw Base::FileException("Cannot read STEP file", file);
    }

    aReader.Transfer(hDoc);
}
