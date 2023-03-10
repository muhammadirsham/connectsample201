//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_MESH_ADAPTER_H
#define PXR_USD_IMAGING_USD_IMAGING_MESH_ADAPTER_H

/// \file usdImaging/meshAdapter.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"
#include "pxr/usdImaging/usdImaging/gprimAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE


class HdSubdivTags;
class PxOsdSubdivTags;

/// \class UsdImagingMeshAdapter
///
/// Delegate support for UsdGeomMesh.
///
class UsdImagingMeshAdapter : public UsdImagingGprimAdapter {
public:
    typedef UsdImagingGprimAdapter BaseAdapter;

    UsdImagingMeshAdapter()
        : UsdImagingGprimAdapter()
    {}
    USDIMAGING_API
    virtual ~UsdImagingMeshAdapter();

    USDIMAGING_API
    virtual SdfPath Populate(UsdPrim const& prim,
                     UsdImagingIndexProxy* index,
                     UsdImagingInstancerContext const* instancerContext = NULL) override;

    USDIMAGING_API
    virtual bool IsSupported(UsdImagingIndexProxy const* index) const override;

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //


    /// Thread Safe.
    USDIMAGING_API
    virtual void TrackVariability(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  HdDirtyBits* timeVaryingBits,
                                  UsdImagingInstancerContext const* 
                                      instancerContext = NULL,
                                  // #nv begin fast-updates
                                  // If checkVariabilty is false, this method
                                  // only populates the value cache with initial values.
                                  bool checkVariability = true) const override;
                                  // nv end

    /// Thread Safe.
    USDIMAGING_API
    virtual void UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext = NULL) const override;

    //+NV_CHANGE FRZHANG
    /////+++Update Skinning Animation API
    USDIMAGING_API
        void UpdateRestPoints(UsdPrim const& prim, SdfPath const& cachePath, UsdTimeCode time,
            const VtVec3fArray& restPoints);

    USDIMAGING_API
        void UpdateSkinningBinding(UsdPrim const& prim, SdfPath const& cachePath, UsdTimeCode time,
            const GfMatrix4d& bindTransform,
            const VtIntArray& jointIndices, const VtFloatArray& jointweights,
            int numInfluencesPerPoint, bool hasConstantInfluences,
            const TfToken& skinningMethod, const VtFloatArray& skinningBlendweights, bool hasConstantSkinningBlendWeights
        );

    USDIMAGING_API
        void UpdateSkelAnim(UsdPrim const& prim, SdfPath const& cachePath, UsdTimeCode time,
            const VtMatrix4fArray& skelAnim, const GfMatrix4d& primWorldToLocal, const GfMatrix4d& skelLocalToWorld
        );
    //-NV_CHANGE FRZHANG

    // ---------------------------------------------------------------------- //
    /// \name Change Processing
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    virtual HdDirtyBits ProcessPropertyChange(UsdPrim const& prim,
                                              SdfPath const& cachePath,
                                              TfToken const& propertyName) override;

    // ---------------------------------------------------------------------- //
    /// \name Data access
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    virtual PxOsdSubdivTags GetSubdivTags(UsdPrim const& usdPrim,
                                          SdfPath const& cachePath,
                                          UsdTimeCode time) const override;

protected:
    USDIMAGING_API
    virtual bool _IsBuiltinPrimvar(TfToken const& primvarName) const override;

private:
    void _GetMeshTopology(UsdPrim const& prim, VtValue* topoHolder, 
            UsdTimeCode time) const;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_MESH_ADAPTER_H
