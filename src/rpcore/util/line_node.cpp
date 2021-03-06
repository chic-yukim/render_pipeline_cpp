/**
 * Render Pipeline C++
 *
 * Copyright (c) 2016-2017 Center of Human-centered Interaction for Coexistence.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "render_pipeline/rpcore/util/line_node.hpp"

#include "render_pipeline/rpcore/render_pipeline.hpp"

namespace rpcore {

const Effect::SourceType LineNode::vertex_color_line_effect_source = { "/$$rp/effects/vcolor_line.yaml", {} };
const Effect::SourceType LineNode::line_effect_source = { "/$$rp/effects/line.yaml", {} };

LineNode::LineNode(NodePath np) : np_(np)
{
}

void LineNode::set_vertex_color_line_effect(RenderPipeline& pipeline)
{
    pipeline.set_effect(np_, vertex_color_line_effect_source);
}

void LineNode::set_line_effect(RenderPipeline& pipeline)
{
    if (pipeline.is_stereo_mode())
        pipeline.set_effect(np_, line_effect_source);
}

}