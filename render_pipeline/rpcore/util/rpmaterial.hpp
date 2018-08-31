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

#include <material.h>

namespace rpcore {

/**
 * Decorator of Panda3D Material for physically based material.
 */
class RPMaterial
{
public:
    enum class ShadingModel: int8_t
    {
        DEFAULT_MODEL = 0,
        EMISSIVE_MODEL,
        CLEARCOAT_MODEL,
        TRANSPARENT_MODEL,
        SKIN_MODEL,
        FOLIAGE_MODEL,
        UNLIT_MODEL,
    };

public:
    /** Create default material. */
    RPMaterial();

    /** Wrapping given material. */
    RPMaterial(Material* material);

    Material& operator*() const;
    Material* operator->() const;
    explicit operator bool() const;
    bool operator!() const;

    Material* get_material() const;

    const LColor& get_base_color() const;
    float get_specular_ior() const;
    float get_metallic() const;
    float get_roughness() const;
    ShadingModel get_shading_model() const;
    float get_normal_factor() const;
    float get_arbitrary0() const;

    /** Used in only TRANSPARENT_MODEL mode. */
    float get_alpha() const;
    bool is_alpha_texture_mode() const;

    void set_material(Material* mat);

    void set_default();
    void set_base_color(const LColor& color);

    /** @param[in]  specular_ior    [1.0, 2.51] */
    void set_specular_ior(float specular_ior);

    /** @param[in]  metallic    [0, 1] */
    void set_metallic(float metallic);

    /** @param[in]  roughness   [0, 1] */
    void set_roughness(float roughness);

    void set_shading_model(ShadingModel shading_model);

    /** @param[in]  normal_factor [0, 1] */
    void set_normal_factor(float normal_factor);

    void set_arbitrary0(float arbitrary0);

    /**
     * Set alpha value.
     *
     * This is used in only TRANSPARENT_MODEL mode.
     *
     * @param[in]   alpha   [0, 1]
     */
    void set_alpha(float alpha);

    /**
     * Use a texture (RGBA, alaph texture) for alpha value.
     *
     * This is used in only TRANSPARENT_MODEL mode.
     */
    void set_alpha_texture_mode();

    void output(std::ostream& out) const;

private:
    PT(Material) material_;
};

// ************************************************************************************************
inline RPMaterial::RPMaterial(): material_(new Material)
{
    set_default();
}

inline RPMaterial::RPMaterial(Material* material): material_(material)
{
}

inline Material& RPMaterial::operator*() const
{
    return *material_;
}

inline Material* RPMaterial::operator->() const
{
    return material_.p();
}

inline RPMaterial::operator bool() const
{
    return material_ != nullptr;
}

inline bool RPMaterial::operator!() const
{
    return material_ == nullptr;
}

inline Material* RPMaterial::get_material() const
{
    return material_;
}

inline const LColor& RPMaterial::get_base_color() const
{
    return material_->get_base_color();
}

inline float RPMaterial::get_specular_ior() const
{
    return material_->get_refractive_index();
}

inline float RPMaterial::get_metallic() const
{
    return material_->get_metallic();
}

inline float RPMaterial::get_roughness() const
{
    return material_->get_roughness();
}

inline RPMaterial::ShadingModel RPMaterial::get_shading_model() const
{
    return static_cast<ShadingModel>(static_cast<int8_t>(material_->get_emission().get_x()));
}

inline float RPMaterial::get_normal_factor() const
{
    return material_->get_emission().get_y();
}

inline float RPMaterial::get_arbitrary0() const
{
    return material_->get_emission().get_z();
}

inline float RPMaterial::get_alpha() const
{
    if (get_shading_model() == ShadingModel::TRANSPARENT_MODEL)
        return get_arbitrary0();
    return 1.0f;
}

inline bool RPMaterial::is_alpha_texture_mode() const
{
    if (get_shading_model() == ShadingModel::TRANSPARENT_MODEL)
        return get_arbitrary0() == 2.0f;
    return false;
}

inline void RPMaterial::set_material(Material* mat)
{
    material_ = mat;
}

inline void RPMaterial::set_default()
{
    set_shading_model(ShadingModel::DEFAULT_MODEL);
    set_base_color(LColor(0.8f, 0.8f, 0.8f, 1.0f));
    set_normal_factor(0.0f);
    set_roughness(0.3f);
    set_specular_ior(1.51f);
    set_metallic(0.0f);
    set_arbitrary0(0.0f);
}

inline void RPMaterial::set_base_color(const LColor& color)
{
    material_->set_base_color(color);
}

inline void RPMaterial::set_specular_ior(float specular_ior)
{
    material_->set_refractive_index((std::max)(1.0f, (std::min)(2.51f, specular_ior)));
}

inline void RPMaterial::set_metallic(float metallic)
{
    material_->set_metallic((std::max)(0.0f, (std::min)(1.0f, metallic)));
}

inline void RPMaterial::set_roughness(float roughness)
{
    material_->set_roughness((std::max)(0.0f, (std::min)(1.0f, roughness)));
}

inline void RPMaterial::set_shading_model(ShadingModel shading_model)
{
    LColor e = material_->get_emission();
    e.set_x(static_cast<int8_t>(shading_model));
    material_->set_emission(e);
}

inline void RPMaterial::set_normal_factor(float normal_factor)
{
    LColor e = material_->get_emission();
    e.set_y((std::max)(0.0f, (std::min)(1.0f, normal_factor)));
    material_->set_emission(e);
}

inline void RPMaterial::set_arbitrary0(float arbitrary0)
{
    LColor e = material_->get_emission();
    e.set_z(arbitrary0);
    material_->set_emission(e);
}

inline void RPMaterial::set_alpha(float alpha)
{
    if (get_shading_model() == ShadingModel::TRANSPARENT_MODEL)
        set_arbitrary0((std::max)(0.0f, (std::min)(1.0f, alpha)));
}

inline void RPMaterial::set_alpha_texture_mode()
{
    if (get_shading_model() == ShadingModel::TRANSPARENT_MODEL)
        set_arbitrary0(2.0f);
}

inline void RPMaterial::output(std::ostream& out) const
{
    out << "RPMaterial " << material_->get_name()
        << " shading_model(" << static_cast<int>(get_shading_model()) << ")"
        << " base_color(" << get_base_color() << ")"
        << " normal_factor(" << get_normal_factor() << ")"
        << " roughness(" << get_roughness() << ")"
        << " specular_ior(" << get_specular_ior() << ")"
        << " metallic(" << get_metallic() << ")"
        << " arbitrary0(" << get_arbitrary0() << ")";
}

// ************************************************************************************************

inline bool operator==(const RPMaterial& a, const RPMaterial& b)
{
    return a.get_material() == b.get_material();
}

inline bool operator!=(const RPMaterial& a, const RPMaterial& b)
{
    return a.get_material() != b.get_material();
}

inline bool operator==(const RPMaterial& m, std::nullptr_t)
{
    return m.get_material() == nullptr;
}

inline bool operator==(std::nullptr_t, const RPMaterial& m)
{
    return m.get_material() == nullptr;
}

inline bool operator!=(const RPMaterial& m, std::nullptr_t)
{
    return m.get_material() != nullptr;
}

inline bool operator!=(std::nullptr_t, const RPMaterial& m)
{
    return m.get_material() != nullptr;
}

inline std::ostream& operator<<(std::ostream& out, const RPMaterial& m)
{
    m.output(out);
    return out;
}

}
