/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#if defined(MAESTRO_EDITOR)

#include <AzCore/PlatformDef.h>

/////////////////////////////////////////////////////////////////////////////
// Engine
/////////////////////////////////////////////////////////////////////////////
#include <Cry_Math.h>
#include <ISystem.h>
#include <ISerialize.h>
#include <CryName.h>

/////////////////////////////////////////////////////////////////////////////
// STL
/////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>

/////////////////////////////////////////////////////////////////////////////
// Qt
/////////////////////////////////////////////////////////////////////////////
#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#else // defined(MAESTRO_EDITOR)

#include <platform.h> // Many CryCommon files require that this is included first.

#endif // defined(MAESTRO_EDITOR)
