#pragma once

#include <tuple>
#include <utility>

#include "datapod/pods/sequential/vector.hpp"
#include "datapod/types/types.hpp"
#include "joint.hpp"
#include "link.hpp"
#include "transmission.hpp"

namespace datapod {
    namespace robot {

        /**
         * @brief Model - Robot kinematic model (POD)
         *
         * Represents a complete robot model with links, joints, and kinematic tree structure.
         * Used for URDF/SDF-style robot definitions with parent-child relationships.
         *
         * The model maintains:
         * - links: All links in the robot
         * - joints: All joints connecting links
         * - root: ID of the root link (typically base_link)
         * - parent_of: Parent link ID for each link
         * - joint_from_parent: Joint ID connecting each link to its parent
         * - children_of: Child link IDs for each link
         */
        struct Model {
            Vector<Link> links;
            Vector<Joint> joints;
            Vector<Transmission> transmissions;
            u32 root = kInvalidId;
            Vector<u32> parent_of;
            Vector<u32> joint_from_parent;
            Vector<Vector<u32>> children_of;

            auto members() noexcept {
                return std::tie(links, joints, transmissions, root, parent_of, joint_from_parent, children_of);
            }
            auto members() const noexcept {
                return std::tie(links, joints, transmissions, root, parent_of, joint_from_parent, children_of);
            }

            /// Add a link to the model, returns its ID
            inline u32 add_link(Link l) {
                u32 id = static_cast<u32>(links.size());
                links.push_back(std::move(l));
                parent_of.push_back(kInvalidId);
                joint_from_parent.push_back(kInvalidId);
                children_of.push_back({});
                if (root == kInvalidId) {
                    root = id;
                }
                return id;
            }

            /// Add a joint to the model, returns its ID
            inline u32 add_joint(Joint j) {
                u32 id = static_cast<u32>(joints.size());
                joints.push_back(std::move(j));
                return id;
            }

            /// Connect parent and child links via a joint
            inline void connect(u32 parent, u32 child, u32 joint_id) {
                joints[joint_id].parent = parent;
                joints[joint_id].child = child;
                parent_of[child] = parent;
                joint_from_parent[child] = joint_id;
                children_of[parent].push_back(child);
            }

            /// Get the number of links
            inline usize num_links() const noexcept { return links.size(); }

            /// Get the number of joints
            inline usize num_joints() const noexcept { return joints.size(); }

            /// Check if a link ID is valid
            inline bool is_valid_link(u32 id) const noexcept { return id < links.size(); }

            /// Check if a joint ID is valid
            inline bool is_valid_joint(u32 id) const noexcept { return id < joints.size(); }

            /// Get parent link ID (kInvalidId if root)
            inline u32 get_parent(u32 link_id) const noexcept {
                return link_id < parent_of.size() ? parent_of[link_id] : kInvalidId;
            }

            /// Get joint connecting link to its parent (kInvalidId if root)
            inline u32 get_parent_joint(u32 link_id) const noexcept {
                return link_id < joint_from_parent.size() ? joint_from_parent[link_id] : kInvalidId;
            }

            /// Get children of a link
            inline const Vector<u32> &get_children(u32 link_id) const noexcept {
                static const Vector<u32> empty;
                return link_id < children_of.size() ? children_of[link_id] : empty;
            }

            /// Check if link is a leaf (no children)
            inline bool is_leaf(u32 link_id) const noexcept {
                return link_id < children_of.size() && children_of[link_id].empty();
            }

            /// Check if link is root
            inline bool is_root(u32 link_id) const noexcept { return link_id == root; }
        };

        namespace model {
            /// Create an empty model
            inline Model make() noexcept { return Model{}; }
        } // namespace model

    } // namespace robot
} // namespace datapod
