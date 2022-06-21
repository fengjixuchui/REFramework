#include "REFramework.hpp"
#include "sdk/SceneManager.hpp"
#include "sdk/RETypeDB.hpp"
#include "sdk/REManagedObject.hpp"
#include "sdk/Renderer.hpp"

#if TDB_VER < 69
#include "sdk/regenny/re3/via/motion/Chain.hpp"
#include "sdk/regenny/re3/via/motion/ChainCollisions.hpp"
#elif TDB_VER == 69 || (TDB_VER == 70 && defined(MHRISE))
#include "sdk/regenny/re8/via/motion/Chain.hpp"
#include "sdk/regenny/re8/via/motion/ChainCollisionTop.hpp"
#include "sdk/regenny/re8/via/motion/ChainCollisions.hpp"
#else
#include "sdk/regenny/re2_tdb70/via/motion/Chain.hpp"
#include "sdk/regenny/re2_tdb70/via/motion/ChainCollisionTop.hpp"
#include "sdk/regenny/re2_tdb70/via/motion/ChainCollisions.hpp"
#endif

#include "ObjectExplorer.hpp"
#include "ChainViewer.hpp"

std::optional<std::string> ChainViewer::on_initialize() {

    // OK
    return Mod::on_initialize();
}

void ChainViewer::on_config_load(const utility::Config& cfg) {
    for (IModValue& option : m_options) {
        option.config_load(cfg);
    }
}

void ChainViewer::on_config_save(utility::Config& cfg) {
    for (IModValue& option : m_options) {
        option.config_save(cfg);
    }
}


void ChainViewer::on_draw_dev_ui() {
    ImGui::SetNextTreeNodeOpen(false, ImGuiCond_::ImGuiCond_Once);

    if (!ImGui::CollapsingHeader(get_name().data())) {
        return;
    }

    ImGui::TextWrapped("Currently only fully functional in RE2, RE3, and DMC5.");

    if (m_enabled->draw("Enabled") && !m_enabled->value()) {
        // todo
    }
}

void ChainViewer::on_frame() {
    if (!m_enabled->value()) {
        return;
    }

    auto scene = sdk::get_current_scene();

    if (scene == nullptr) {
        return;
    }

    auto context = sdk::get_thread_context();

    static auto scene_def = sdk::find_type_definition("via.Scene");
    auto first_transform = sdk::call_native_func_easy<RETransform*>(scene, scene_def, "get_FirstTransform");

    if (first_transform == nullptr) {
        return;
    }

    static auto transform_def = utility::re_managed_object::get_type_definition(first_transform);
    static auto next_transform_method = transform_def->get_method("get_Next");
    static auto child_transform_method = transform_def->get_method("get_Child");
    static auto get_gameobject_method = transform_def->get_method("get_GameObject");

    auto camera = sdk::get_primary_camera();

    if (camera == nullptr) {
        return;
    }

    auto camera_gameobject = get_gameobject_method->call<::REGameObject*>(sdk::get_thread_context(), camera);

    if (camera_gameobject == nullptr) {
        return;
    }

    auto camera_transform = camera_gameobject->transform;

    if (camera_transform == nullptr) {
        return;
    }

    auto camera_joints = sdk::call_object_func_easy<sdk::SystemArray*>(camera_transform, "get_Joints");

    if (camera_joints == nullptr) {
        return;
    }

    auto camera_joint = (::REJoint*)camera_joints->get_element(0);

    if (camera_joint == nullptr) {
        return;
    }

    const auto camera_pos = sdk::get_joint_position(camera_joint);
    //const auto camera_forward = sdk::get_joint_rotation(camera_joint) * Vector3f{ 0.0f, 0.0f, 1.0f };
    const auto camera_up = Vector3f{glm::mat4{sdk::get_joint_rotation(camera_joint)}[1]};

    ImGui::Begin("Chains");

    for (auto transform = first_transform; 
        transform != nullptr; 
        transform = next_transform_method->call<RETransform*>(context, transform)) 
    {
        auto attempt_display_chains = [&](RETransform* transform) {
            auto owner = get_gameobject_method->call<REGameObject*>(context, transform);

            if (owner == nullptr) {
                return;
            }

            auto owner_name = utility::re_string::get_string(owner->name);

            if (owner_name.empty()) {
                return;
            }

            static auto chain_type = sdk::find_type_definition("via.motion.Chain");
            static auto chain_re_type = chain_type->get_type();

            auto chain = utility::re_component::find<regenny::via::motion::Chain>(transform, chain_re_type);
            bool made = false;

            if (chain == nullptr) {
                return;
            }

            ImGui::PushID(chain);
            made = ImGui::TreeNode(chain, owner_name.data());

            if (made) {
                ObjectExplorer::get()->handle_address(chain);
            }

            if (chain != nullptr && chain->CollisionData.num > 0 && chain->CollisionData.collisions != nullptr) {
                for (auto i = 0; i < chain->CollisionData.num; ++i) {
                    #if TDB_VER >= 69
                    const auto& collider_top = chain->CollisionData.collisions[i];

                    for (auto j = 0; j < collider_top.num_collisions; ++j) {
                        const auto& collider = collider_top.collisions[j];
                    #else
                        const auto& collider = chain->CollisionData.collisions[i];
                    #endif
                        auto pos = *(Vector4f*)&collider.capsule.p0;

                        if (collider.joint != nullptr) {
                            //pos = sdk::get_joint_position((::REJoint*)collider.joint);
                        }

                        if (collider.joint != nullptr) {
                            //const auto joint_pos = Vector3f{sdk::get_joint_position((::REJoint*)collider.joint)};
                            const auto joint_pos = *(Vector3f*)&collider.sphere.pos;
                            const auto joint_screen_pos_center = sdk::renderer::world_to_screen(joint_pos);

                            if (joint_screen_pos_center) {
                                const auto joint_pos_top = joint_pos + (glm::normalize(camera_up) * collider.sphere.r);
                                const auto joint_screen_pos_top = sdk::renderer::world_to_screen(joint_pos_top);

                                if (joint_screen_pos_top) {
                                    const auto radius2d = glm::length(*joint_screen_pos_top - *joint_screen_pos_center);

                                    ImGui::GetBackgroundDrawList()->AddCircleFilled(
                                        *(ImVec2*)&*joint_screen_pos_center,
                                        radius2d,
                                        ImGui::GetColorU32(ImVec4(66.0f / 255.0f, 105.0f / 255.0f, 245.0f / 255.0f, 0.25f)),
                                        32
                                    );

                                    // Draw a 2D circle.
                                    /*for (auto f = 0; f < 360; f++) {
                                        auto r1 = f * (glm::pi<float>() / 180.0f);
                                        auto x1 = joint_screen_pos_center->x + radius2d * cos(r1);
                                        auto y1 = joint_screen_pos_center->y + radius2d * sin(r1);

                                        auto r2 = (f + 1) * (glm::pi<float>() / 180.0f);
                                        auto x2 = joint_screen_pos_center->x + radius2d * cos(r2);
                                        auto y2 = joint_screen_pos_center->y + radius2d * sin(r2);
                                        
                                        ImGui::GetBackgroundDrawList()->AddLine(
                                            ImVec2{x1, y1},
                                            ImVec2{x2, y2},
                                            ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f))
                                        );
                                    }*/
                                }
                            }
                        }

                        //world_to_screen_methods[1]->call<void*>(&screen_pos, context, &pos, &view, &proj, &screen_size);
                        //draw_list->AddText(ImVec2(screen_pos.x, screen_pos.y), ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), "Collision");

                        if (made) {
                            ImGui::PushID(&collider);
                            
                            ImGui::SetNextTreeNodeOpen(true, ImGuiCond_::ImGuiCond_Once);

                        #if TDB_VER >= 69
                            if (ImGui::TreeNode(&collider, "Collision %d %d", i, j)) {
                        #else
                            if (ImGui::TreeNode(&collider, "Collision %d", i)) {
                        #endif
                                if (ImGui::TreeNode(&collider.joint, "Joint")) {
                                    ObjectExplorer::get()->handle_address(collider.joint);
                                    ImGui::TreePop();
                                }

                                if (ImGui::TreeNode(&collider.pair_joint, "Pair Joint")) {
                                    ObjectExplorer::get()->handle_address(collider.pair_joint);
                                    ImGui::TreePop();
                                }

                                ImGui::DragFloat("Radius", (float*)&collider.radius, 0.01f, 0.0f, 0.0f);
                                ImGui::TreePop();
                            }

                            ImGui::PopID();
                        }
                #if TDB_VER >= 69
                    }
                #endif
                }
            }

            if (made) {
                ImGui::TreePop();
            }

            ImGui::PopID();
        };

        attempt_display_chains(transform);

        for (auto t = child_transform_method->call<RETransform*>(context, transform); t != nullptr; t = next_transform_method->call<RETransform*>(context, t)) {
            attempt_display_chains(t);

            for (auto t2 = child_transform_method->call<RETransform*>(context,t); t2 != nullptr; t2 = next_transform_method->call<RETransform*>(context, t2)) {
                attempt_display_chains(t2);
            }
        }
    }

    ImGui::End();
}

