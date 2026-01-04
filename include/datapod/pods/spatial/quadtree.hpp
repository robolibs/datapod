#pragma once
#include <datapod/types/types.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <tuple>

#include "aabb.hpp"
#include "datapod/pods/sequential/vector.hpp"
#include "point.hpp"

namespace datapod {

    /**
     * @brief QuadTree spatial index for 2D point data
     *
     * Efficient spatial data structure for 2D point queries with logarithmic
     * performance for insertion, removal, and range queries.
     *
     * Features:
     * - Hierarchical subdivision (4 children: NW, NE, SW, SE)
     * - Capacity-based splitting (default: 16 entries per node)
     * - Range queries (AABB)
     * - Radius queries (circular)
     * - k-Nearest Neighbor search
     * - Insert/Remove operations
     * - Full serialization support via members()
     * - POD-compatible
     *
     * @tparam T The type of data stored with each point
     * @tparam Capacity Maximum entries per node before splitting (default: 16)
     */
    template <typename T, datapod::usize Capacity = 16> struct QuadTree {
        struct Entry {
            Point point;
            T data;

            auto members() noexcept { return std::tie(point, data); }
            auto members() const noexcept { return std::tie(point, data); }

            bool operator==(const Entry &other) const noexcept { return point == other.point && data == other.data; }

            bool operator<(const Entry &other) const noexcept {
                if (point.x != other.point.x)
                    return point.x < other.point.x;
                if (point.y != other.point.y)
                    return point.y < other.point.y;
                return point.z < other.point.z;
            }
        };

      private:
        struct Node {
            AABB boundary;
            Vector<Entry> entries;
            Vector<Node> children; // 4 children: NW, NE, SW, SE (empty if leaf)

            auto members() noexcept { return std::tie(boundary, entries, children); }
            auto members() const noexcept { return std::tie(boundary, entries, children); }

            inline bool is_leaf() const noexcept { return children.empty(); }

            void subdivide() {
                Point center = boundary.center();

                // Reserve space for 4 children
                children.reserve(4);

                // Create quadrants: NW, NE, SW, SE
                // NW: min_x to center_x, center_y to max_y
                children.push_back(Node{AABB{Point{boundary.min_point.x, center.y, boundary.min_point.z},
                                             Point{center.x, boundary.max_point.y, boundary.max_point.z}},
                                        Vector<Entry>{}, Vector<Node>{}});

                // NE: center_x to max_x, center_y to max_y
                children.push_back(Node{AABB{Point{center.x, center.y, boundary.min_point.z},
                                             Point{boundary.max_point.x, boundary.max_point.y, boundary.max_point.z}},
                                        Vector<Entry>{}, Vector<Node>{}});

                // SW: min_x to center_x, min_y to center_y
                children.push_back(Node{AABB{Point{boundary.min_point.x, boundary.min_point.y, boundary.min_point.z},
                                             Point{center.x, center.y, boundary.max_point.z}},
                                        Vector<Entry>{}, Vector<Node>{}});

                // SE: center_x to max_x, min_y to center_y
                children.push_back(Node{AABB{Point{center.x, boundary.min_point.y, boundary.min_point.z},
                                             Point{boundary.max_point.x, center.y, boundary.max_point.z}},
                                        Vector<Entry>{}, Vector<Node>{}});
            }
        };

        Node root_;

        // Insert helper
        bool insert(Node &node, const Entry &entry) {
            if (!node.boundary.contains(entry.point)) {
                return false;
            }

            if (node.entries.size() < Capacity && node.is_leaf()) {
                node.entries.push_back(entry);
                return true;
            }

            if (node.is_leaf()) {
                node.subdivide();

                // Redistribute existing entries
                Vector<Entry> old_entries = std::move(node.entries);
                node.entries.clear();

                for (const auto &old_entry : old_entries) {
                    bool inserted = false;
                    for (datapod::usize i = 0; i < 4; ++i) {
                        if (insert(node.children[i], old_entry)) {
                            inserted = true;
                            break;
                        }
                    }
                    if (!inserted) {
                        node.entries.push_back(old_entry); // Keep in parent if doesn't fit children
                    }
                }
            }

            // Try to insert in children
            for (datapod::usize i = 0; i < 4; ++i) {
                if (insert(node.children[i], entry)) {
                    return true;
                }
            }

            // If all children reject, keep in this node
            node.entries.push_back(entry);
            return true;
        }

        // Query helper (range)
        void query(const Node &node, const AABB &range, Vector<Entry> &results) const {
            if (!node.boundary.intersects(range)) {
                return;
            }

            for (const auto &entry : node.entries) {
                if (range.contains(entry.point)) {
                    results.push_back(entry);
                }
            }

            if (!node.is_leaf()) {
                for (datapod::usize i = 0; i < 4; ++i) {
                    query(node.children[i], range, results);
                }
            }
        }

        // Query radius helper
        void query_radius(const Node &node, const Point &center, double radius, Vector<Entry> &results) const {
            // Quick AABB check first
            AABB query_box{Point{center.x - radius, center.y - radius, center.z - radius},
                           Point{center.x + radius, center.y + radius, center.z + radius}};

            if (!node.boundary.intersects(query_box)) {
                return;
            }

            double radius_sq = radius * radius;

            for (const auto &entry : node.entries) {
                double dx = entry.point.x - center.x;
                double dy = entry.point.y - center.y;
                if (dx * dx + dy * dy <= radius_sq) {
                    results.push_back(entry);
                }
            }

            if (!node.is_leaf()) {
                for (datapod::usize i = 0; i < 4; ++i) {
                    query_radius(node.children[i], center, radius, results);
                }
            }
        }

        // Remove helper
        bool remove(Node &node, const Point &point, const T &data) {
            if (!node.boundary.contains(point)) {
                return false;
            }

            // Check entries in this node
            for (datapod::usize i = 0; i < node.entries.size(); ++i) {
                if (node.entries[i].point == point && node.entries[i].data == data) {
                    // Remove by swapping with last and popping
                    node.entries[i] = node.entries.back();
                    node.entries.pop_back();
                    return true;
                }
            }

            // Check children
            if (!node.is_leaf()) {
                for (datapod::usize i = 0; i < 4; ++i) {
                    if (remove(node.children[i], point, data)) {
                        return true;
                    }
                }
            }

            return false;
        }

        // k-Nearest helper
        void k_nearest(const Node &node, const Point &point, datapod::usize k,
                       Vector<std::pair<double, Entry>> &candidates) const {
            // Add all entries from this node
            for (const auto &entry : node.entries) {
                double dx = entry.point.x - point.x;
                double dy = entry.point.y - point.y;
                double distance_sq = dx * dx + dy * dy;
                candidates.push_back({distance_sq, entry});
            }

            if (!node.is_leaf()) {
                // Sort children by distance to query point
                Vector<std::pair<double, datapod::usize>> child_distances;
                child_distances.reserve(4);

                for (datapod::usize i = 0; i < 4; ++i) {
                    double min_dist = node.children[i].boundary.distance_to_point(point);
                    child_distances.push_back({min_dist * min_dist, i});
                }

                std::sort(child_distances.begin(), child_distances.end());

                for (const auto &[dist, child_idx] : child_distances) {
                    k_nearest(node.children[child_idx], point, k, candidates);
                }
            }
        }

        // Count entries helper
        datapod::usize count_entries(const Node &node) const {
            datapod::usize count = node.entries.size();
            if (!node.is_leaf()) {
                for (datapod::usize i = 0; i < 4; ++i) {
                    count += count_entries(node.children[i]);
                }
            }
            return count;
        }

        // Collect all entries helper
        void collect_all(const Node &node, Vector<Entry> &all_entries) const {
            for (const auto &entry : node.entries) {
                all_entries.push_back(entry);
            }
            if (!node.is_leaf()) {
                for (datapod::usize i = 0; i < 4; ++i) {
                    collect_all(node.children[i], all_entries);
                }
            }
        }

      public:
        // Construction
        QuadTree(const AABB &boundary) : root_{boundary, Vector<Entry>{}, Vector<Node>{}} {}

        // Serialization
        auto members() noexcept { return std::tie(root_); }
        auto members() const noexcept { return std::tie(root_); }

        /**
         * @brief Insert a point with associated data
         * @param point The point to insert
         * @param data The data to associate with the point
         * @return True if insertion was successful
         */
        inline bool insert(const Point &point, const T &data) {
            Entry entry{point, data};
            return insert(root_, entry);
        }

        inline bool insert(const Entry &entry) { return insert(root_, entry); }

        /**
         * @brief Query for all points within a rectangular region
         * @param range The AABB defining the query region
         * @return Vector of entries within the region
         */
        inline Vector<Entry> query(const AABB &range) const {
            Vector<Entry> results;
            query(root_, range, results);
            return results;
        }

        /**
         * @brief Query for all points within a radius of a center point
         * @param center The center point for the circular query
         * @param radius The search radius
         * @return Vector of entries within the radius
         */
        inline Vector<Entry> query_radius(const Point &center, double radius) const {
            Vector<Entry> results;
            query_radius(root_, center, radius, results);
            return results;
        }

        /**
         * @brief Remove a specific point and data combination
         * @param point The point to remove
         * @param data The data to remove
         * @return True if the entry was found and removed
         */
        inline bool remove(const Point &point, const T &data) { return remove(root_, point, data); }

        inline bool remove(const Entry &entry) { return remove(entry.point, entry.data); }

        /**
         * @brief Find k nearest neighbors to a point
         * @param point The query point
         * @param k Number of neighbors to find
         * @return Vector of k nearest entries (may be less than k if tree has fewer entries)
         */
        inline Vector<Entry> k_nearest(const Point &point, datapod::usize k) const {
            Vector<std::pair<double, Entry>> candidates;
            k_nearest(root_, point, k, candidates);

            // Sort by distance and take first k
            std::sort(candidates.begin(), candidates.end());

            Vector<Entry> results;
            datapod::usize count = std::min(k, candidates.size());
            results.reserve(count);
            for (datapod::usize i = 0; i < count; ++i) {
                results.push_back(candidates[i].second);
            }
            return results;
        }

        /**
         * @brief Get the total number of entries in the QuadTree
         */
        inline datapod::usize size() const { return count_entries(root_); }

        /**
         * @brief Check if the QuadTree is empty
         */
        inline bool empty() const { return size() == 0; }

        /**
         * @brief Clear all entries from the QuadTree
         */
        inline void clear() {
            root_.entries.clear();
            root_.children.clear();
        }

        /**
         * @brief Get the boundary of the QuadTree
         */
        inline const AABB &boundary() const { return root_.boundary; }

        // Iterator support
        struct Iterator {
            Vector<Entry> entries;
            datapod::usize index;

            const Entry &operator*() const { return entries[index]; }
            const Entry *operator->() const { return &entries[index]; }

            Iterator &operator++() {
                ++index;
                return *this;
            }

            bool operator!=(const Iterator &other) const { return index != other.index; }
            bool operator==(const Iterator &other) const { return index == other.index; }
        };

        inline Iterator begin() const {
            Iterator it;
            collect_all(root_, it.entries);
            it.index = 0;
            return it;
        }

        inline Iterator end() const {
            Iterator it;
            collect_all(root_, it.entries);
            it.index = it.entries.size();
            return it;
        }
    };

    namespace quadtree {
        /// Placeholder for template container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace quadtree

} // namespace datapod
