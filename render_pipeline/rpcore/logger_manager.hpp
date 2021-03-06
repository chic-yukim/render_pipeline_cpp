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

#pragma once

#include <spdlog/tweakme.h>
#include <spdlog/logger.h>

#include <render_pipeline/rpcore/config.hpp>

class Filename;

namespace rpcore {

class RENDER_PIPELINE_DECL LoggerManager
{
public:
    static LoggerManager& get_instance();

    LoggerManager();
    ~LoggerManager();

    spdlog::logger* get_logger() const;

    /**
     * Create internal logger.
     * If @p file_path is empty, file logging will be disabled.
     */
    void create(const Filename& file_path);

    bool is_created() const;

    void clear();

private:
    std::shared_ptr<spdlog::logger> logger_;
    std::mutex mutex_;
};

}
