#include "mem_console_ui_graph_internal.h"

#include <math.h>

float graph_clampf(float value, float min_v, float max_v) {
    if (value < min_v) return min_v;
    if (value > max_v) return max_v;
    return value;
}

static void graph_route_nearest_point_with_tangent(const KitGraphStructEdgeRoute *route,
                                                   KitRenderVec2 target,
                                                   KitRenderVec2 *out_point,
                                                   KitRenderVec2 *out_tangent) {
    float best_dist_sq = 0.0f;
    int has_best = 0;
    uint32_t p;

    if (!route || route->point_count < 2u || !out_point || !out_tangent) {
        return;
    }

    for (p = 0u; p + 1u < route->point_count; ++p) {
        KitRenderVec2 a = route->points[p];
        KitRenderVec2 b = route->points[p + 1u];
        float vx = b.x - a.x;
        float vy = b.y - a.y;
        float seg_len_sq = (vx * vx) + (vy * vy);
        float t = 0.0f;
        float cx;
        float cy;
        float dx;
        float dy;
        float dist_sq;

        if (seg_len_sq <= 0.00001f) {
            continue;
        }

        t = ((target.x - a.x) * vx + (target.y - a.y) * vy) / seg_len_sq;
        t = graph_clampf(t, 0.0f, 1.0f);
        cx = a.x + (vx * t);
        cy = a.y + (vy * t);
        dx = target.x - cx;
        dy = target.y - cy;
        dist_sq = (dx * dx) + (dy * dy);

        if (!has_best || dist_sq < best_dist_sq) {
            float inv_len = 1.0f / sqrtf(seg_len_sq);
            best_dist_sq = dist_sq;
            has_best = 1;
            *out_point = (KitRenderVec2){ cx, cy };
            *out_tangent = (KitRenderVec2){ vx * inv_len, vy * inv_len };
        }
    }

    if (!has_best) {
        KitRenderVec2 a = route->points[0];
        KitRenderVec2 b = route->points[1];
        float vx = b.x - a.x;
        float vy = b.y - a.y;
        float len_sq = (vx * vx) + (vy * vy);
        float inv_len = len_sq > 0.00001f ? (1.0f / sqrtf(len_sq)) : 1.0f;
        *out_point = a;
        *out_tangent = (KitRenderVec2){ vx * inv_len, vy * inv_len };
    }
}

static int graph_point_in_rect(KitRenderRect rect, float x, float y) {
    return (x >= rect.x && x <= rect.x + rect.width &&
            y >= rect.y && y <= rect.y + rect.height);
}

static float graph_orient2d(KitRenderVec2 a, KitRenderVec2 b, KitRenderVec2 c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

static int graph_on_segment(KitRenderVec2 a, KitRenderVec2 b, KitRenderVec2 p) {
    const float eps = 0.0001f;
    if (p.x < (a.x < b.x ? a.x : b.x) - eps || p.x > (a.x > b.x ? a.x : b.x) + eps) return 0;
    if (p.y < (a.y < b.y ? a.y : b.y) - eps || p.y > (a.y > b.y ? a.y : b.y) + eps) return 0;
    return 1;
}

static int graph_segments_intersect(KitRenderVec2 a0,
                                    KitRenderVec2 a1,
                                    KitRenderVec2 b0,
                                    KitRenderVec2 b1) {
    float o1 = graph_orient2d(a0, a1, b0);
    float o2 = graph_orient2d(a0, a1, b1);
    float o3 = graph_orient2d(b0, b1, a0);
    float o4 = graph_orient2d(b0, b1, a1);
    const float eps = 0.0001f;

    if (((o1 > eps && o2 < -eps) || (o1 < -eps && o2 > eps)) &&
        ((o3 > eps && o4 < -eps) || (o3 < -eps && o4 > eps))) {
        return 1;
    }
    if (fabsf(o1) <= eps && graph_on_segment(a0, a1, b0)) return 1;
    if (fabsf(o2) <= eps && graph_on_segment(a0, a1, b1)) return 1;
    if (fabsf(o3) <= eps && graph_on_segment(b0, b1, a0)) return 1;
    if (fabsf(o4) <= eps && graph_on_segment(b0, b1, a1)) return 1;
    return 0;
}

static int graph_segment_intersects_rect(KitRenderVec2 a,
                                         KitRenderVec2 b,
                                         KitRenderRect rect) {
    KitRenderVec2 r0 = { rect.x, rect.y };
    KitRenderVec2 r1 = { rect.x + rect.width, rect.y };
    KitRenderVec2 r2 = { rect.x + rect.width, rect.y + rect.height };
    KitRenderVec2 r3 = { rect.x, rect.y + rect.height };

    if (graph_point_in_rect(rect, a.x, a.y) || graph_point_in_rect(rect, b.x, b.y)) {
        return 1;
    }
    if (graph_segments_intersect(a, b, r0, r1)) return 1;
    if (graph_segments_intersect(a, b, r1, r2)) return 1;
    if (graph_segments_intersect(a, b, r2, r3)) return 1;
    if (graph_segments_intersect(a, b, r3, r0)) return 1;
    return 0;
}

static int graph_label_rect_edge_overlap_count(const KitGraphStructEdgeRoute *routes,
                                               uint32_t route_count,
                                               KitRenderRect rect) {
    uint32_t i;
    int overlap_count = 0;
    const float edge_pad = 1.0f;
    KitRenderRect test_rect = {
        rect.x - edge_pad,
        rect.y - edge_pad,
        rect.width + (edge_pad * 2.0f),
        rect.height + (edge_pad * 2.0f)
    };

    for (i = 0u; i < route_count; ++i) {
        const KitGraphStructEdgeRoute *route = &routes[i];
        uint32_t p;
        int hit = 0;

        if (route->point_count < 2u) {
            continue;
        }
        for (p = 0u; p + 1u < route->point_count; ++p) {
            if (graph_segment_intersects_rect(route->points[p], route->points[p + 1u], test_rect)) {
                hit = 1;
                break;
            }
        }
        if (hit) {
            overlap_count += 1;
        }
    }
    return overlap_count;
}

static int graph_rects_overlap(KitRenderRect a,
                               KitRenderRect b,
                               float pad) {
    float ax0 = a.x - pad;
    float ay0 = a.y - pad;
    float ax1 = a.x + a.width + pad;
    float ay1 = a.y + a.height + pad;
    float bx0 = b.x - pad;
    float by0 = b.y - pad;
    float bx1 = b.x + b.width + pad;
    float by1 = b.y + b.height + pad;

    if (ax1 <= bx0 || bx1 <= ax0) return 0;
    if (ay1 <= by0 || by1 <= ay0) return 0;
    return 1;
}

static int graph_label_rect_label_overlap_count(const KitGraphStructEdgeLabelLayout *label_layouts,
                                                uint32_t placed_count,
                                                KitRenderRect rect) {
    uint32_t i;
    int overlap_count = 0;
    const float label_pad = 2.0f;

    if (!label_layouts || placed_count == 0u) {
        return 0;
    }

    for (i = 0u; i < placed_count; ++i) {
        const KitRenderRect other = label_layouts[i].rect;
        if (other.width <= 0.0f || other.height <= 0.0f) {
            continue;
        }
        if (graph_rects_overlap(other, rect, label_pad)) {
            overlap_count += 1;
        }
    }

    return overlap_count;
}

void refine_edge_label_layouts_for_callouts(KitRenderRect bounds,
                                            const KitGraphStructEdgeRoute *routes,
                                            uint32_t route_count,
                                            KitGraphStructEdgeLabelLayout *label_layouts) {
    uint32_t i;

    if (!routes || !label_layouts) {
        return;
    }

    for (i = 0u; i < route_count; ++i) {
        const KitGraphStructEdgeRoute *route = &routes[i];
        KitGraphStructEdgeLabelLayout *layout = &label_layouts[i];
        KitRenderRect rect = layout->rect;
        KitRenderVec2 target = layout->anchor;
        KitRenderVec2 anchor = {0.0f, 0.0f};
        KitRenderVec2 tangent = {1.0f, 0.0f};
        KitRenderVec2 normal;
        float normal_len;
        float width;
        float height;
        float center_x;
        float center_y;
        float to_center_x;
        float to_center_y;
        float side_dot;
        const float callout_gap = 10.0f;
        const float pad = 2.0f;
        float min_x;
        float max_x;
        float min_y;
        float max_y;
        float best_score = 1e9f;
        int best_label_overlap_count = 2147483647;
        int best_overlap_count = 2147483647;
        uint32_t side_index;
        uint32_t dist_index;
        uint32_t lateral_index;
        const float distance_opts[] = { 10.0f, 16.0f, 24.0f, 34.0f, 46.0f };
        const float lateral_opts[] = { 0.0f, 10.0f, -10.0f, 18.0f, -18.0f, 28.0f, -28.0f };
        KitRenderRect best_rect = rect;
        int found_zero_overlap = 0;

        if (route->point_count < 2u || rect.width <= 0.0f || rect.height <= 0.0f) {
            continue;
        }

        if (target.x == 0.0f && target.y == 0.0f) {
            target.x = rect.x + (rect.width * 0.5f);
            target.y = rect.y + (rect.height * 0.5f);
        }

        graph_route_nearest_point_with_tangent(route, target, &anchor, &tangent);
        normal = (KitRenderVec2){ -tangent.y, tangent.x };
        normal_len = sqrtf((normal.x * normal.x) + (normal.y * normal.y));
        if (normal_len <= 0.00001f) {
            normal = (KitRenderVec2){0.0f, -1.0f};
        } else {
            normal.x /= normal_len;
            normal.y /= normal_len;
        }

        center_x = rect.x + (rect.width * 0.5f);
        center_y = rect.y + (rect.height * 0.5f);
        to_center_x = center_x - anchor.x;
        to_center_y = center_y - anchor.y;
        side_dot = (to_center_x * normal.x) + (to_center_y * normal.y);
        if (side_dot < 0.0f) {
            normal.x = -normal.x;
            normal.y = -normal.y;
        } else if (fabsf(side_dot) < 0.01f && (i & 1u)) {
            normal.x = -normal.x;
            normal.y = -normal.y;
        }

        width = rect.width;
        height = rect.height;

        min_x = bounds.x + pad;
        max_x = bounds.x + bounds.width - width - pad;
        min_y = bounds.y + pad;
        max_y = bounds.y + bounds.height - height - pad;
        if (max_x < min_x) max_x = min_x;
        if (max_y < min_y) max_y = min_y;

        for (side_index = 0u; side_index < 2u; ++side_index) {
            KitRenderVec2 trial_normal = normal;
            if (side_index == 1u) {
                trial_normal.x = -trial_normal.x;
                trial_normal.y = -trial_normal.y;
            }

            for (dist_index = 0u; dist_index < (uint32_t)(sizeof(distance_opts) / sizeof(distance_opts[0])); ++dist_index) {
                for (lateral_index = 0u; lateral_index < (uint32_t)(sizeof(lateral_opts) / sizeof(lateral_opts[0])); ++lateral_index) {
                    KitRenderRect trial_rect;
                    float dist = distance_opts[dist_index] + callout_gap;
                    float lateral = lateral_opts[lateral_index];
                    float score;
                    int overlaps;
                    int label_overlaps;

                    center_x = anchor.x + (trial_normal.x * dist) + (tangent.x * lateral);
                    center_y = anchor.y + (trial_normal.y * dist) + (tangent.y * lateral);

                    trial_rect.width = width;
                    trial_rect.height = height;
                    trial_rect.x = graph_clampf(center_x - (width * 0.5f), min_x, max_x);
                    trial_rect.y = graph_clampf(center_y - (height * 0.5f), min_y, max_y);

                    overlaps = graph_label_rect_edge_overlap_count(routes, route_count, trial_rect);
                    label_overlaps = graph_label_rect_label_overlap_count(label_layouts, i, trial_rect);
                    score = (float)overlaps * 10000.0f +
                            (float)label_overlaps * 16000.0f +
                            dist * 1.0f +
                            fabsf(lateral) * 0.2f +
                            (side_index == 1u ? 0.5f : 0.0f);

                    if (overlaps == 0 && label_overlaps == 0) {
                        if (!found_zero_overlap || score < best_score) {
                            found_zero_overlap = 1;
                            best_score = score;
                            best_rect = trial_rect;
                        }
                    } else if (!found_zero_overlap) {
                        if (label_overlaps < best_label_overlap_count ||
                            (label_overlaps == best_label_overlap_count &&
                             (overlaps < best_overlap_count ||
                              (overlaps == best_overlap_count && score < best_score)))) {
                            best_label_overlap_count = label_overlaps;
                            best_overlap_count = overlaps;
                            best_score = score;
                            best_rect = trial_rect;
                        }
                    }
                }
            }
        }

        layout->rect = best_rect;
        layout->anchor = anchor;
    }
}

KitRenderVec2 compute_label_attach_point(KitRenderRect rect, KitRenderVec2 anchor) {
    KitRenderVec2 out = {
        graph_clampf(anchor.x, rect.x, rect.x + rect.width),
        graph_clampf(anchor.y, rect.y, rect.y + rect.height)
    };

    if (anchor.x >= rect.x && anchor.x <= rect.x + rect.width &&
        anchor.y >= rect.y && anchor.y <= rect.y + rect.height) {
        float left_d = anchor.x - rect.x;
        float right_d = (rect.x + rect.width) - anchor.x;
        float top_d = anchor.y - rect.y;
        float bottom_d = (rect.y + rect.height) - anchor.y;
        float best = left_d;
        out = (KitRenderVec2){ rect.x, anchor.y };
        if (right_d < best) {
            best = right_d;
            out = (KitRenderVec2){ rect.x + rect.width, anchor.y };
        }
        if (top_d < best) {
            best = top_d;
            out = (KitRenderVec2){ anchor.x, rect.y };
        }
        if (bottom_d < best) {
            out = (KitRenderVec2){ anchor.x, rect.y + rect.height };
        }
    }

    return out;
}
