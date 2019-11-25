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

// include required headers
#include "OBB.h"
#include "AABB.h"

#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobContext.h>

namespace MCore
{
    // check if the box contains a given point
    bool OBB::Contains(const AZ::Vector3& p) const
    {
        // translate to box space
        AZ::Vector3 relPoint = p - mCenter;

        // convert the box into box space and test each axis
        float f = MMAT(mRotation, 0, 0) * relPoint.GetX() + MMAT(mRotation, 0, 1) * relPoint.GetY() + MMAT(mRotation, 0, 2) * relPoint.GetZ();
        if (f >= mExtents.GetX() || f <= -mExtents.GetX())
        {
            return false;
        }

        f = MMAT(mRotation, 1, 0) * relPoint.GetX() + MMAT(mRotation, 1, 1) * relPoint.GetY() + MMAT(mRotation, 1, 2) * relPoint.GetZ();
        if (f >= mExtents.GetY() || f <= -mExtents.GetY())
        {
            return false;
        }

        f = MMAT(mRotation, 2, 0) * relPoint.GetX() + MMAT(mRotation, 2, 1) * relPoint.GetY() + MMAT(mRotation, 2, 2) * relPoint.GetZ();
        if (f >= mExtents.GetZ() || f <= -mExtents.GetZ())
        {
            return false;
        }

        return true;
    }


    void OBB::Create(const AABB& aabb, const Matrix& mat)
    {
        // calculate the center and extents
        mCenter  = aabb.CalcMiddle();
        mExtents = aabb.CalcExtents();

        // transform the center
        mCenter = mCenter * mat;

        // set the rotation
        mRotation = mat;
    }


    void OBB::Transform(const Matrix& transMatrix)
    {
        mCenter   = mCenter * transMatrix;
        mRotation = mRotation * transMatrix;
    }


    void OBB::Transformed(const Matrix& transMatrix, OBB* outOBB) const
    {
        outOBB->mExtents  = mExtents;
        outOBB->mCenter   = mCenter * transMatrix;
        outOBB->mRotation = mRotation * transMatrix;
    }


    bool OBB::CheckIfIsInside(const OBB& box) const
    {
        // make a 4x4 from the box & inverse it
        Matrix M0 = box.mRotation;
        M0.SetTranslation(box.mCenter);
        Matrix M0Inv = M0.Inversed();

        // with our inversed 4x4, create box1 in space of box0
        OBB _1in0;
        Transformed(M0Inv, &_1in0);

        // this should cancel out box0's rotation, i.e. it's now an AABB

        // the two boxes are in the same space so now we can compare them
        // create the AABB of (box1 in space of box0)
        const Matrix& mtx = _1in0.mRotation;

        float f = Math::Abs(MMAT(mtx, 0, 0) * mExtents.GetX()) + Math::Abs(MMAT(mtx, 1, 0) * mExtents.GetY()) + Math::Abs(MMAT(mtx, 2, 0) * mExtents.GetZ()) - box.mExtents.GetX();
        if (f > _1in0.mCenter.GetX())
        {
            return false;
        }
        if (-f < _1in0.mCenter.GetX())
        {
            return false;
        }

        f = Math::Abs(MMAT(mtx, 0, 1) * mExtents.GetX()) + Math::Abs(MMAT(mtx, 1, 1) * mExtents.GetY()) + Math::Abs(MMAT(mtx, 2, 1) * mExtents.GetZ()) - box.mExtents.GetY();
        if (f > _1in0.mCenter.GetY())
        {
            return false;
        }
        if (-f < _1in0.mCenter.GetY())
        {
            return false;
        }

        f = Math::Abs(MMAT(mtx, 0, 2) * mExtents.GetX()) + Math::Abs(MMAT(mtx, 1, 2) * mExtents.GetY()) + Math::Abs(MMAT(mtx, 2, 2) * mExtents.GetZ()) - box.mExtents.GetZ();
        if (f > _1in0.mCenter.GetZ())
        {
            return false;
        }
        if (-f < _1in0.mCenter.GetZ())
        {
            return false;
        }

        return true;
    }


    // calculate the corner points for the OBB
    void OBB::CalcCornerPoints(AZ::Vector3* outPoints) const
    {
        MCORE_ASSERT(outPoints);
        MCORE_ASSERT(CheckIfIsValid());

        AZ::Vector3 right   = mRotation.GetRight();
        AZ::Vector3 up      = mRotation.GetUp();
        AZ::Vector3 forward = mRotation.GetForward();

        right   *= mExtents.GetX();
        up      *= mExtents.GetZ();
        forward *= mExtents.GetY();

        //     7+------+6
        //     /|     /|
        //    / |    / |
        //   / 4+---/--+5
        // 3+------+2 /
        //  | /    | /
        //  |/     |/
        // 0+------+1

        outPoints[0] = mCenter - right - up - forward;
        outPoints[1] = mCenter + right - up - forward;
        outPoints[2] = mCenter + right + up - forward;
        outPoints[3] = mCenter - right + up - forward;
        outPoints[4] = mCenter - right - up + forward;
        outPoints[5] = mCenter + right - up + forward;
        outPoints[6] = mCenter + right + up + forward;
        outPoints[7] = mCenter - right + up + forward;
    }


    //----------------------------------------------------------------------------------------------------------

    // calculate the 3 eigen vectors
    void OBB::GetRealSymmetricEigenvectors(const float A[6], AZ::Vector3& v1, AZ::Vector3& v2, AZ::Vector3& v3)
    {
        // compute coefficients for cubic equation
        const float c2 = A[0] + A[3] + A[5];
        const float a12sq = A[1] * A[1];
        const float a13sq = A[2] * A[2];
        const float a23sq = A[4] * A[4];
        const float a11a22 = A[0] * A[3];
        const float c1 = a11a22 - a12sq + A[0] * A[5] - a13sq + A[3] * A[5] - a23sq;
        const float c0 = a11a22 * A[5] + 2.0f * A[1] * A[2] * A[4] - A[0] * a23sq - A[3] * a13sq - A[5] * a12sq;

        // compute intermediate values for root solving
        const float c2sq = c2 * c2;
        const float a = (3.0f * c1 - c2sq) / 3.0f;
        const float b = (9.0f * c1 * c2 - 2.0f * c2sq * c2 - 27.f * c0) / 27.0f;
        const float halfb = b * 0.5f;
        const float halfb2 = halfb * halfb;
        const float Q = halfb2 + a * a * a / 27.0f;

        // determine type of eigenspaces
        if (Q > 1.0e-6f)
        {
            // one eigenvalue, use standard basis
            v1.Set(1.0f, 0.0f, 0.0f);
            v2.Set(0.0f, 1.0f, 0.0f);
            v3.Set(0.0f, 0.0f, 1.0f);
            return;
        }
        else
        if (Q < -1.0e-6f)
        {
            // three distinct eigenvalues

            // intermediate terms
            const float theta_3 = Math::ATan2(Math::Sqrt(-Q), -halfb) / 3.0f;
            float rho = Math::Sqrt(halfb2 - Q);
            const float c2_3 = c2 / 3.0f;
            float rho_13 = powf(Math::Abs(rho), 1.0f / 3.0f);
            if (rho < 0.0f)
            {
                rho_13 = -rho_13;
            }
            float ct_3, st_3;
            const float sqrt3 = Math::Sqrt(3.0f);
            ct_3 = Math::Cos(theta_3);
            st_3 = Math::Sin(theta_3);

            // compute each eigenvalue and eigenvector
            // sort from largest to smallest
            float lambda1 = c2_3 + 2.0f * rho_13 * ct_3;
            CalcSymmetricEigenVector(A, lambda1, v1);

            float lambda2 = c2_3 - rho_13 * (ct_3 + sqrt3 * st_3);
            if (lambda2 > lambda1)
            {
                v2 = v1;
                float temp = lambda2;
                lambda2 = lambda1;
                lambda1 = temp;
                CalcSymmetricEigenVector(A, lambda2, v1);
            }
            else
            {
                CalcSymmetricEigenVector(A, lambda2, v2);
            }

            float lambda3 = c2_3 - rho_13 * (ct_3 - sqrt3 * st_3);
            if (lambda3 > lambda1)
            {
                v3 = v2;
                v2 = v1;
                CalcSymmetricEigenVector(A, lambda3, v1);
            }
            else
            if (lambda3 > lambda2)
            {
                v3 = v2;
                CalcSymmetricEigenVector(A, lambda3, v2);
            }
            else
            {
                CalcSymmetricEigenVector(A, lambda3, v3);
            }
        }
        else
        {
            // two distinct eigenvalues

            // intermediate terms
            float c2_3 = c2 / 3.0f;
            float halfb_13 = Math::Pow(Math::Abs(halfb), 1.0f / 3.0f);
            if (halfb < 0.0f)
            {
                halfb_13 = -halfb_13;
            }

            // compute each eigenvalue and eigenvector
            // sort from largest to smallest
            float lambda1 = c2_3 + halfb_13;
            CalcSymmetricEigenPair(A, lambda1, v1, v2);

            float lambda2 = c2_3 - 2.0f * halfb_13;
            if (lambda2 > lambda1)
            {
                v3 = v2;
                v2 = v1;
                CalcSymmetricEigenVector(A, lambda2, v1);
            }
            else
            {
                CalcSymmetricEigenVector(A, lambda2, v3);
            }
        }

        v1.Normalize();
        v2.Normalize();
        v3.Normalize();

        if ((v1.Cross(v2)).Dot(v3) < 0.0f)
        {
            v3 = -v3;
        }
    }


    // calculate the eigen vector from a symmetric matrix in combination with a given eigen value
    void OBB::CalcSymmetricEigenVector(const float A[6], float eigenValue, AZ::Vector3& v1)
    {
        const float m11 = A[0] - eigenValue;
        const float m12 = A[1];
        const float m13 = A[2];
        const float m22 = A[3] - eigenValue;
        const float m23 = A[4];
        const float m33 = A[5] - eigenValue;

        // compute cross product matrix, and find column with maximal entry
        const float u11 = m22 * m33 - m23 * m23;
        float max = Math::Abs(u11);
        int c = 1;
        const float u12 = m13 * m23 - m12 * m33;
        if (Math::Abs(u12) > max)
        {
            max = Math::Abs(u12);
            c = 2;
        }

        const float u13 = m12 * m23 - m13 * m22;
        if (Math::Abs(u13) > max)
        {
            max = Math::Abs(u13);
            c = 3;
        }

        const float u22 = m11 * m33 - m13 * m13;
        if (Math::Abs(u22) > max)
        {
            max = Math::Abs(u22);
            c = 2;
        }

        const float u23 = m12 * m13 - m23 * m11;
        if (Math::Abs(u23) > max)
        {
            max = Math::Abs(u23);
            c = 3;
        }

        const float u33 = m11 * m22 - m12 * m12;
        if (Math::Abs(u33) > max)
        {
            max = Math::Abs(u33);
            c = 3;
        }

        // return column with maximal entry
        if (c == 1)
        {
            v1.Set(u11, u12, u13);
        }
        else
        if (c == 2)
        {
            v1.Set(u12, u22, u23);
        }
        else
        {
            v1.Set(u13, u23, u33);
        }
    }


    //-------------------------------------------------------------------------------
    // Given symmetric matrix A and eigenvalue l, returns eigenvector pair
    // Assumes that order of eigenvalue is 2
    //-------------------------------------------------------------------------------
    void OBB::CalcSymmetricEigenPair(const float A[6], float eigenValue, AZ::Vector3& v1, AZ::Vector3& v2)
    {
        // find maximal entry in M
        const float m11 = A[0] - eigenValue;
        float max = Math::Abs(m11);
        int r = 1, c = 1;
        if (Math::Abs(A[1]) > max)
        {
            max = Math::Abs(A[1]);
            r = 1;
            c = 2;
        }

        if (Math::Abs(A[2]) > max)
        {
            max = Math::Abs(A[2]);
            r = 1;
            c = 3;
        }

        const float m22 = A[3] - eigenValue;
        if (Math::Abs(m22) > max)
        {
            max = Math::Abs(m22);
            r = 2;
            c = 2;
        }

        if (Math::Abs(A[4]) > max)
        {
            max = Math::Abs(A[4]);
            r = 2;
            c = 3;
        }

        const float m33 = A[5] - eigenValue;
        if (Math::Abs(m33) > max)
        {
            r = 3;
            c = 3;
        }

        // compute eigenvectors for each case
        if (r == 1)
        {
            if (c == 3)
            {
                v1.Set(A[2], 0.0f, -m11);
                v2.Set(-A[1] * m11, m11 * m11 + A[2] * A[2], -A[1] * A[2]);
            }
            else
            {
                v1.Set(-A[1], m11, 0.0f);
                v2.Set(-A[2] * m11, -A[2] * A[1], m11 * m11 + A[1] * A[1]);
            }
        }
        else
        if (r == 2)
        {
            v1.Set(0.0f, -A[4], m22);
            v2.Set(m22 * m22 + A[4] * A[4], -A[1] * m22, -A[1] * A[4]);
        }
        else
        if (r == 3)
        {
            v1.Set(0.0f, -m33, A[4]);
            v2.Set(A[4] * A[4] + m33 * m33, -A[2] * A[4], -A[2] * m33);
        }
    }

    //-----------------------

    //-------------------------------------------------------------------------------
    // Compute covariance matrix for set of points
    // Returns centroid and unique values of matrix
    //-------------------------------------------------------------------------------
    void OBB::CovarianceMatrix(const AZ::Vector3* points, uint32 numPoints, AZ::Vector3& mean, float C[6])
    {
        uint32 i;

        // compute mean
        mean = points[0];
        for (i = 1; i < numPoints; ++i)
        {
            mean += points[i];
        }

        mean *= 1.0f / numPoints;

        // compute each element of matrix
        memset(C, 0, sizeof(float) * 6);
        for (i = 0; i < numPoints; ++i)
        {
            const AZ::Vector3 diff = points[i] - mean;
            C[0] += diff.GetX() * diff.GetX();
            C[1] += diff.GetX() * diff.GetY();
            C[2] += diff.GetX() * diff.GetZ();
            C[3] += diff.GetY() * diff.GetY();
            C[4] += diff.GetY() * diff.GetZ();
            C[5] += diff.GetZ() * diff.GetZ();
        }

        // normalize the matrix values
        float maxC = 0.0f;
        for (i = 0; i < 6; ++i)
        {
            if (Math::Abs(C[i]) > maxC)
            {
                maxC = Math::Abs(C[i]);
            }
        }
        for (i = 0; i < 6; ++i)
        {
            C[i] /= maxC;
        }
    }


    // calc the best fit for a given x rotation slice
    void OBB::InitFromPointsRange(const AZ::Vector3* points, uint32 numPoints, float xDegrees, float* outMinArea, AABB* outMinBox, Matrix* outMinMatrix)
    {
        // calculate the x rotation matrix
        MCore::Matrix rotMatrix;
        rotMatrix.SetRotationMatrixX(Math::DegreesToRadians(xDegrees));

        // try the same over the z axis
        for (float z = -180.0f; z < 180.0f; z += 5.0f)
        {
            // calculate the final rotation matrix
            rotMatrix.RotateZ(Math::DegreesToRadians(z));

            // calculate the transposed, which is the same as the inverse as there is only rotation
            // we need this so we can transform the point set into space of this current rotation
            MCore::Matrix invMatrix = rotMatrix;
            invMatrix.Transpose();

            // rotate the points into the space of the current rotation
            AABB box;
            box.Init();
            for (uint32 i = 0; i < numPoints; ++i)
            {
                box.Encapsulate(invMatrix.Mul3x3(points[i]));
            }

            // check if the surface area of this box is smaller than the smallest one we have
            const float area = box.CalcSurfaceArea();
            if (area < *outMinArea)
            {
                *outMinArea     = area;
                *outMinBox      = box;
                *outMinMatrix   = rotMatrix;
            }
        }
    }


    // Compute bounding box for set of points
    void OBB::InitFromPoints(const AZ::Vector3* points, uint32 numPoints)
    {
        // if we have no points, just init
        if (numPoints == 0)
        {
            Init();
            return;
        }

        // some values we need
        const uint32 MAX_NUM = (360 / 5) + 1;
        AABB    minBoxes[MAX_NUM];
        Matrix  minRotMatrices[MAX_NUM];
        float   minAreas[MAX_NUM];
        for (uint32 i = 0; i < MAX_NUM; ++i)
        {
            minAreas[i] = FLT_MAX;
        }


        // try all rotation on the x axis (multithreaded)
        AZ::JobCompletion jobCompletion;           
        uint32 index = 0;
        for (float x = -180.0f; x < 180.0f; x += 5.0f)
        {
            MCORE_ASSERT(index < MAX_NUM);

            // create the job and add it
            AZ::JobContext* jobContext = nullptr;
            AZ::Job* job = AZ::CreateJobFunction([this, &minAreas, &minBoxes, &minRotMatrices, &numPoints, &points, x, index]()
            {   
                InitFromPointsRange(points, numPoints, x, &minAreas[index], &minBoxes[index], &minRotMatrices[index]);
            }, true, jobContext);

            job->SetDependent(&jobCompletion);
            job->Start();

            index++;
        }

        jobCompletion.StartAndWaitForCompletion();

        // find the real minimum value (single threaded lookup)
        float minimumArea = FLT_MAX;
        uint32 minimumIndex = 0;
        for (uint32 i = 0; i < MAX_NUM; ++i)
        {
            if (minAreas[i] < minimumArea)
            {
                minimumArea     = minAreas[i];
                minimumIndex    = i;
            }
        }

        // update
        mRotation   = minRotMatrices[minimumIndex];
        mCenter     = mRotation.Mul3x3(minBoxes[minimumIndex].CalcMiddle());
        mExtents    = minBoxes[minimumIndex].CalcExtents();

        /*
            // compute covariance matrix
            float C[6];
            CovarianceMatrix( points, numPoints, mCenter, C );

            // get principle axes
            Vector3 basis[3];
            GetRealSymmetricEigenvectors( C, basis[0], basis[1], basis[2] );

            // init the min and max vectors
            Vector3 minVec;
            Vector3 maxVec;
            minVec.Set(FLT_MAX, FLT_MAX, FLT_MAX);
            maxVec.Set(-FLT_MAX, -FLT_MAX, -FLT_MAX);

            // find the min and max
            for (uint32 i=0; i<numPoints; ++i)
            {
                Vector3 diff = points[i] - mCenter;
                for (int32 j=0; j<3; ++j)
                {
                    const float length = diff.Dot( basis[j] );
                    if (length > maxVec[j])
                        maxVec[j] = length;
                    else
                    if (length < minVec[j])
                        minVec[j] = length;
                }
            }

            // build the matrix from the calculated basis vectors
            mRotation.Identity();
            mRotation.SetRow(0, basis[0]);
            mRotation.SetRow(1, basis[1]);
            mRotation.SetRow(2, basis[2]);

            // calculate the extents
            mExtents = (maxVec - minVec) * 0.5f;
        */
    }


    // calculate the minimum and maximum point
    void OBB::CalcMinMaxPoints(AZ::Vector3* outMin, AZ::Vector3* outMax) const
    {
        AZ::Vector3 rotatedExtents = mRotation.Mul3x3(mExtents);
        *outMax = mCenter + rotatedExtents;
        *outMin = mCenter - rotatedExtents;

        //       +------+MAX
        //      /|     /|
        //     / |    / |
        //    /  +---/--+
        //   +------+  /
        //   | /    | /
        //   |/     |/
        //MIN+------+
    }
}   // namespace MCore
