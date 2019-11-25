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

#include "ActorFixture.h"
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/SimulatedObjectCommands.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>


namespace EMotionFX
{
    class SimulatedObjectCommandTests
        : public ActorFixture
    {
    public:
        void SetUp()
        {
            ActorFixture::SetUp();
        }

        void TearDown()
        {
            ActorFixture::TearDown();
        }

        size_t CountSimulatedObjects(const Actor* actor)
        {
            const AZStd::shared_ptr<SimulatedObjectSetup>& simulatedObjectSetup = actor->GetSimulatedObjectSetup();
            return simulatedObjectSetup->GetNumSimulatedObjects();
        }

        size_t CountSimulatedJoints(const Actor* actor, size_t objectIndex)
        {
            const AZStd::shared_ptr<SimulatedObjectSetup>& simulatedObjectSetup = actor->GetSimulatedObjectSetup();
            const SimulatedObject* object = simulatedObjectSetup->GetSimulatedObject(objectIndex);
            return object->GetNumSimulatedJoints();
        }

        size_t CountChildJoints(const Actor* actor, size_t objectIndex, AZ::u32 jointIndex)
        {
            const AZStd::shared_ptr<SimulatedObjectSetup>& simulatedObjectSetup = actor->GetSimulatedObjectSetup();
            const SimulatedObject* object = simulatedObjectSetup->GetSimulatedObject(objectIndex);
            const SimulatedJoint* joint = object->FindSimulatedJointBySkeletonJointIndex(jointIndex);
            return joint->CalculateNumChildSimulatedJoints();
        }

        size_t CountRootJoints(const Actor* actor, size_t objectIndex)
        {
            const AZStd::shared_ptr<SimulatedObjectSetup>& simulatedObjectSetup = actor->GetSimulatedObjectSetup();
            const SimulatedObject* object = simulatedObjectSetup->GetSimulatedObject(objectIndex);
            return object->GetNumSimulatedRootJoints();
        }
    };


    TEST_F(SimulatedObjectCommandTests, EvaluateSimulatedObjectCommands)
    {
        AZStd::string result;
        CommandSystem::CommandManager commandManager;
        MCore::CommandGroup commandGroup;

        const AZ::u32 actorId = m_actor->GetID();
        const AZStd::vector<AZStd::string> jointNames = GetTestJointNames();
        const size_t jointCount = jointNames.size();

        // 1. Add simulated object.
        const AZStd::string serializedBeforeAdd = SerializeSimulatedObjectSetup(m_actor);
        CommandSimulatedObjectHelpers::AddSimulatedObject(actorId, AZStd::nullopt, &commandGroup);
        CommandSimulatedObjectHelpers::AddSimulatedObject(actorId, AZStd::nullopt, &commandGroup);
        CommandSimulatedObjectHelpers::AddSimulatedObject(actorId, AZStd::nullopt, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        const AZStd::string serializedAfterAdd = SerializeSimulatedObjectSetup(m_actor);
        EXPECT_EQ(3, CountSimulatedObjects(m_actor));

        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(0, CountSimulatedObjects(m_actor));
        EXPECT_EQ(serializedBeforeAdd, SerializeSimulatedObjectSetup(m_actor));

        EXPECT_TRUE(commandManager.Redo(result));
        EXPECT_EQ(3, CountSimulatedObjects(m_actor));
        EXPECT_EQ(serializedAfterAdd, SerializeSimulatedObjectSetup(m_actor));

        // 2. Remove simulated object.
        commandGroup.RemoveAllCommands();
        CommandSimulatedObjectHelpers::RemoveSimulatedObject(actorId, 2, &commandGroup);
        CommandSimulatedObjectHelpers::RemoveSimulatedObject(actorId, 0, &commandGroup);
        CommandSimulatedObjectHelpers::RemoveSimulatedObject(actorId, 0, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)); 
        EXPECT_EQ(0, CountSimulatedObjects(m_actor));
        EXPECT_EQ(serializedBeforeAdd, SerializeSimulatedObjectSetup(m_actor));

        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(3, CountSimulatedObjects(m_actor));
        EXPECT_EQ(serializedAfterAdd, SerializeSimulatedObjectSetup(m_actor));

        EXPECT_TRUE(commandManager.Redo(result));
        EXPECT_EQ(0, CountSimulatedObjects(m_actor));
        EXPECT_EQ(serializedBeforeAdd, SerializeSimulatedObjectSetup(m_actor));

        // 3. Add simulated joints.
        // 3.1 Add a simulated object first to put in the simulated joints.
        commandGroup.RemoveAllCommands();
        CommandSimulatedObjectHelpers::AddSimulatedObject(actorId);
        const AZStd::string serialized3_1 = SerializeSimulatedObjectSetup(m_actor);

        // 3.2 Add simulated joints.
        // Joint hierarchy as follow:
        // l_upLeg
        //      --l_upLegRoll
        //      --l_loLeg
        //             --l_ankle
        //                    --l_ball
        const Skeleton* skeleton = m_actor->GetSkeleton();
        const AZ::u32 l_upLegIdx = skeleton->FindNodeByName("l_upLeg")->GetNodeIndex();
        const AZ::u32 l_upLegRollIdx = skeleton->FindNodeByName("l_upLegRoll")->GetNodeIndex();
        const AZ::u32 l_loLegIdx = skeleton->FindNodeByName("l_loLeg")->GetNodeIndex();
        const AZ::u32 l_ankleIdx = skeleton->FindNodeByName("l_ankle")->GetNodeIndex();
        const AZ::u32 l_ballIdx = skeleton->FindNodeByName("l_ball")->GetNodeIndex();
        CommandSimulatedObjectHelpers::AddSimulatedJoints(actorId, {l_upLegIdx, l_upLegRollIdx, l_loLegIdx, l_ankleIdx, l_ballIdx}, 0, false, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        const AZStd::string serialized3_2 = SerializeSimulatedObjectSetup(m_actor);
        EXPECT_EQ(5, CountSimulatedJoints(m_actor, 0));
        EXPECT_EQ(2, CountChildJoints(m_actor, 0, l_upLegIdx));
        EXPECT_EQ(0, CountChildJoints(m_actor, 0, l_upLegRollIdx));
        EXPECT_EQ(1, CountChildJoints(m_actor, 0, l_loLegIdx));

        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(0, CountSimulatedJoints(m_actor, 0));
        EXPECT_EQ(serialized3_1, SerializeSimulatedObjectSetup(m_actor));

        EXPECT_TRUE(commandManager.Redo(result));
        EXPECT_EQ(5, CountSimulatedJoints(m_actor, 0));
        EXPECT_EQ(serialized3_2, SerializeSimulatedObjectSetup(m_actor));

        // 4 Remove simulated joints.
        // 4.1 Test sparse chain.
        EXPECT_EQ(1, CountRootJoints(m_actor, 0));
        commandGroup.RemoveAllCommands();
        CommandSimulatedObjectHelpers::RemoveSimulatedJoints(actorId, {l_loLegIdx}, 0, false, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        EXPECT_EQ(2, CountRootJoints(m_actor, 0));

        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(1, CountRootJoints(m_actor, 0));

        EXPECT_TRUE(commandManager.Redo(result));
        EXPECT_EQ(2, CountRootJoints(m_actor, 0));

        EXPECT_TRUE(commandManager.Undo(result));

        // 4.2 Test removing all the joints.
        commandGroup.RemoveAllCommands();
        CommandSimulatedObjectHelpers::RemoveSimulatedJoints(actorId, { l_upLegIdx, l_upLegRollIdx, l_loLegIdx, l_ankleIdx, l_ballIdx }, 0, false, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        EXPECT_EQ(0, CountSimulatedJoints(m_actor, 0));
        EXPECT_EQ(serialized3_1, SerializeSimulatedObjectSetup(m_actor));

        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(5, CountSimulatedJoints(m_actor, 0));
        EXPECT_EQ(serialized3_2, SerializeSimulatedObjectSetup(m_actor));

        // 4.3 Test removing the root joint and children.
        commandGroup.RemoveAllCommands();
        CommandSimulatedObjectHelpers::RemoveSimulatedJoints(actorId, { l_upLegIdx }, 0, true, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        EXPECT_EQ(0, CountSimulatedJoints(m_actor, 0));
        EXPECT_EQ(serialized3_1, SerializeSimulatedObjectSetup(m_actor));

        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(5, CountSimulatedJoints(m_actor, 0));
        EXPECT_EQ(serialized3_2, SerializeSimulatedObjectSetup(m_actor));
    }

    TEST_F(SimulatedObjectCommandTests, SimulatedObjectCommands_UndoRemoveJointTest)
    {
        AZStd::string result;
        CommandSystem::CommandManager commandManager;
        MCore::CommandGroup commandGroup;

        const AZ::u32 actorId = m_actor->GetID();
        const AZStd::vector<AZStd::string> jointNames = GetTestJointNames();
        const size_t jointCount = jointNames.size();

        // 1. Add simulated object
        CommandSimulatedObjectHelpers::AddSimulatedObject(actorId, AZStd::nullopt, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        const AZStd::string serializedBase = SerializeSimulatedObjectSetup(m_actor);
        const size_t simulatedObjectIndex = 0;
        SimulatedObject* simulatedObject = m_actor->GetSimulatedObjectSetup()->GetSimulatedObject(simulatedObjectIndex);

        // 2. Add r_upLeg simulated joints
        const Skeleton* skeleton = m_actor->GetSkeleton();
        const AZ::u32 r_upLegIdx = skeleton->FindNodeByName("r_upLeg")->GetNodeIndex();
        const AZ::u32 r_loLegIdx = skeleton->FindNodeByName("r_loLeg")->GetNodeIndex();
        CommandSimulatedObjectHelpers::AddSimulatedJoints(actorId, { r_upLegIdx, r_loLegIdx }, 0, false);
        EXPECT_EQ(2, CountSimulatedJoints(m_actor, 0));
        const AZStd::string serializedUpLeg = SerializeSimulatedObjectSetup(m_actor);

        // 3. Remove the r_loLeg simulated joint
        SimulatedJoint* simulatedJoint = simulatedObject->FindSimulatedJointBySkeletonJointIndex(r_upLegIdx);
        AZStd::vector<SimulatedJoint*> jointsToBeRemoved;
        commandGroup.RemoveAllCommands();
        CommandSimulatedObjectHelpers::RemoveSimulatedJoints(actorId, { r_upLegIdx }, simulatedObjectIndex, true, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        EXPECT_EQ(0, CountSimulatedJoints(m_actor, 0));
        EXPECT_EQ(serializedBase, SerializeSimulatedObjectSetup(m_actor));

        // 4. Undo
        // This recreates r_loLeg and r_loLeg but won't add all other children recursively as only these two joints got aded in step 3.
        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(2, CountSimulatedJoints(m_actor, 0));
        EXPECT_EQ(serializedUpLeg, SerializeSimulatedObjectSetup(m_actor));
    }
} // namespace EMotionFX
