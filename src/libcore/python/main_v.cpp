#include <mitsuba/core/vector.h>
#include <mitsuba/core/spectrum.h>
#include <mitsuba/python/python.h>

MTS_PY_DECLARE(Object);
MTS_PY_DECLARE(BoundingBox);
MTS_PY_DECLARE(BoundingSphere);
MTS_PY_DECLARE(Frame);
MTS_PY_DECLARE(Ray);
MTS_PY_DECLARE(DiscreteDistribution);
MTS_PY_DECLARE(DiscreteDistribution2D);
MTS_PY_DECLARE(ContinuousDistribution);
MTS_PY_DECLARE(IrregularContinuousDistribution);
MTS_PY_DECLARE(Hierarchical2D);
MTS_PY_DECLARE(Marginal2D);
MTS_PY_DECLARE(math);
MTS_PY_DECLARE(qmc);
MTS_PY_DECLARE(Properties);
MTS_PY_DECLARE(rfilter);
MTS_PY_DECLARE(sample_tea);
// MTS_PY_DECLARE(spline);
MTS_PY_DECLARE(Spectrum);
MTS_PY_DECLARE(Transform);
MTS_PY_DECLARE(AnimatedTransform);
MTS_PY_DECLARE(vector);
MTS_PY_DECLARE(warp);
MTS_PY_DECLARE(xml);

#define MODULE_NAME MTS_MODULE_NAME(core, MTS_VARIANT_NAME)

using Caster = py::object(*)(mitsuba::Object *);
Caster cast_object = nullptr;

PYBIND11_MODULE(MODULE_NAME, m) {
    // Temporarily change the module name (for pydoc)
    m.attr("__name__") = "mitsuba.core";

    MTS_PY_IMPORT_TYPES()

    // Create sub-modules
    py::module math   = create_submodule(m, "math"),
               spline = create_submodule(m, "spline"),
               warp   = create_submodule(m, "warp"),
               xml    = create_submodule(m, "xml");

    math.doc()   = "Mathematical routines, special functions, etc.";
    spline.doc() = "Functions for evaluating and sampling Catmull-Rom splines";
    warp.doc()   = "Common warping techniques that map from the unit square to other "
                   "domains, such as spheres, hemispheres, etc.";
    xml.doc()    = "Mitsuba scene XML parser";

    // Import the right variant of Enoki
    const char *enoki_pkg = nullptr;
    if constexpr (ek::is_cuda_array_v<Float> &&
                  ek::is_diff_array_v<Float>)
        enoki_pkg = "enoki.cuda.ad";
    else if constexpr (ek::is_cuda_array_v<Float>)
        enoki_pkg = "enoki.cuda";
    else
        enoki_pkg = "enoki.scalar";

    // TODO refactoring: handle all the other cases

    py::module enoki        = py::module::import(enoki_pkg);
    py::module enoki_scalar = py::module::import("enoki.scalar");

    // Ensure that 'enoki.dynamic' is loaded in CPU mode (needed for DynamicArray<> casts)
    // if constexpr (!ek::is_cuda_array_v<Float>)
        // py::module::import("enoki.dynamic");

    // Basic type aliases in the Enoki module (scalar + vectorized)
    m.attr("Float32") = enoki.attr("Array1f");
    m.attr("Float64") = enoki.attr("Array1f64");
    m.attr("Mask")    = enoki.attr("Array1b");
    m.attr("Int32")   = enoki.attr("Array1i");
    m.attr("Int64")   = enoki.attr("Array1i64");
    m.attr("UInt32")  = enoki.attr("Array1u");
    m.attr("UInt64")  = enoki.attr("Array1u64");

    m.attr("ScalarFloat32") = enoki_scalar.attr("Array1f");
    m.attr("ScalarFloat64") = enoki_scalar.attr("Array1f64");
    m.attr("ScalarMask")    = enoki_scalar.attr("Array1b");
    m.attr("ScalarInt32")   = enoki_scalar.attr("Array1i");
    m.attr("ScalarInt64")   = enoki_scalar.attr("Array1i64");
    m.attr("ScalarUInt32")  = enoki_scalar.attr("Array1u");
    m.attr("ScalarUInt64")  = enoki_scalar.attr("Array1u64");

    if constexpr (std::is_same_v<float, ScalarFloat>) {
        m.attr("Float") = enoki.attr("Array1f");
        m.attr("ScalarFloat") = enoki_scalar.attr("Array1f");
    } else {
        m.attr("Float") = enoki.attr("Array1f64");
        m.attr("ScalarFloat") = enoki_scalar.attr("Array1f64");
    }

    // Vector type aliases
    for (int dim = 0; dim < 4; ++dim) {
        for (int i = 0; i < 3; ++i) {
            std::string ek_name = "Array" + std::to_string(dim) + "fiu"[i];
            if constexpr (std::is_same_v<double, ScalarFloat>)
                ek_name += "64";

            std::string mts_v_name = "Vector" + std::to_string(dim) + "fiu"[i];
            std::string mts_p_name = "Point" + std::to_string(dim) + "fiu"[i];

            py::handle h = enoki.attr(ek_name.c_str());
            m.attr(mts_v_name.c_str()) = h;
            m.attr(mts_p_name.c_str()) = h;

            h = enoki_scalar.attr(ek_name.c_str());
            m.attr(("Scalar" + mts_v_name).c_str()) = h;
            m.attr(("Scalar" + mts_p_name).c_str()) = h;
        }
    }

    // Matrix type aliases
    for (int dim = 2; dim < 5; ++dim) {
        std::string ek_name = "Matrix" + std::to_string(dim) + "f";
        if constexpr (std::is_same_v<double, ScalarFloat>)
            ek_name += "64";

        std::string mts_d_name = "Matrix" + std::to_string(dim) + "f";

        py::handle h = enoki.attr(ek_name.c_str());
        m.attr(mts_d_name.c_str()) = h;

        h = enoki_scalar.attr(ek_name.c_str());
        m.attr(("Scalar" + mts_d_name).c_str()) = h;
    }

    m.attr("Normal3f")       = m.attr("Vector3f");
    m.attr("ScalarNormal3f") = m.attr("ScalarVector3f");

    m.attr("Color3f")        = m.attr("Vector3f");
    m.attr("ScalarColor3f")  = m.attr("ScalarVector3f");

    m.attr("Color1f")        = m.attr("Vector1f");
    m.attr("ScalarColor1f")  = m.attr("ScalarVector1f");

    if constexpr (ek::is_cuda_array_v<Float> && ek::is_diff_array_v<Float>)
        m.attr("PCG32") = py::module::import("enoki.cuda").attr("PCG32");
    else
        m.attr("PCG32") = enoki.attr("PCG32");

    /* After importing the 'enoki' module, pybind11 is aware
       of various Enoki array types (e.g. Array<Float, 3>), etc.

       Unfortunately, it is completely unaware of Mitsuba-specific array
       variants, including points, vectors, normals, etc. Creating additional
       bindings for that many flavors of vectors would be rather prohibitive,
       so a compromise is made in the Python bindings: we consider types such
       as Vector<Float, 3>, Point<Float, 3>, Array<Float, 3>, etc., to be
       identical. The following lines set up these equivalencies.
     */

    pybind11_type_alias<ek::Array<Float, 1>,  Vector1f>();
    pybind11_type_alias<ek::Array<Float, 1>,  Point1f>();
    pybind11_type_alias<ek::Array<Float, 1>,  Color1f>();
    pybind11_type_alias<ek::Array<Float, 0>,  Color<Float, 0>>();

    pybind11_type_alias<ek::Array<Float, 2>,  Vector2f>();
    pybind11_type_alias<ek::Array<Float, 2>,  Point2f>();
    pybind11_type_alias<ek::Array<Int32, 2>,  Vector2i>();
    pybind11_type_alias<ek::Array<Int32, 2>,  Point2i>();
    pybind11_type_alias<ek::Array<UInt32, 2>, Vector2u>();
    pybind11_type_alias<ek::Array<UInt32, 2>, Point2u>();

    pybind11_type_alias<ek::Array<Float, 3>,  Vector3f>();
    pybind11_type_alias<ek::Array<Float, 3>,  Color3f>();
    pybind11_type_alias<ek::Array<Float, 3>,  Point3f>();
    pybind11_type_alias<ek::Array<Float, 3>,  Normal3f>();
    pybind11_type_alias<ek::Array<Int32, 3>,  Vector3i>();
    pybind11_type_alias<ek::Array<Int32, 3>,  Point3i>();
    pybind11_type_alias<ek::Array<UInt32, 3>, Vector3u>();
    pybind11_type_alias<ek::Array<UInt32, 3>, Point3u>();

    pybind11_type_alias<ek::Array<Float, 4>,  Vector4f>();
    pybind11_type_alias<ek::Array<Float, 4>,  Point4f>();
    pybind11_type_alias<ek::Array<Int32, 4>,  Vector4i>();
    pybind11_type_alias<ek::Array<Int32, 4>,  Point4i>();
    pybind11_type_alias<ek::Array<UInt32, 4>, Vector4u>();
    pybind11_type_alias<ek::Array<UInt32, 4>, Point4u>();

    if constexpr (ek::is_array_v<Float>) {
        pybind11_type_alias<ek::Array<ScalarFloat, 1>,  ScalarVector1f>();
        pybind11_type_alias<ek::Array<ScalarFloat, 1>,  ScalarPoint1f>();
        pybind11_type_alias<ek::Array<ScalarFloat, 1>,  ScalarColor1f>();
        pybind11_type_alias<ek::Array<ScalarFloat, 0>,  Color<ScalarFloat, 0>>();

        pybind11_type_alias<ek::Array<ScalarFloat, 2>,  ScalarVector2f>();
        pybind11_type_alias<ek::Array<ScalarFloat, 2>,  ScalarPoint2f>();
        pybind11_type_alias<ek::Array<ScalarInt32, 2>,  ScalarVector2i>();
        pybind11_type_alias<ek::Array<ScalarInt32, 2>,  ScalarPoint2i>();
        pybind11_type_alias<ek::Array<ScalarUInt32, 2>, ScalarVector2u>();
        pybind11_type_alias<ek::Array<ScalarUInt32, 2>, ScalarPoint2u>();

        pybind11_type_alias<ek::Array<ScalarFloat, 3>,  ScalarVector3f>();
        pybind11_type_alias<ek::Array<ScalarFloat, 3>,  ScalarColor3f>();
        pybind11_type_alias<ek::Array<ScalarFloat, 3>,  ScalarPoint3f>();
        pybind11_type_alias<ek::Array<ScalarFloat, 3>,  ScalarNormal3f>();
        pybind11_type_alias<ek::Array<ScalarInt32, 3>,  ScalarVector3i>();
        pybind11_type_alias<ek::Array<ScalarInt32, 3>,  ScalarPoint3i>();
        pybind11_type_alias<ek::Array<ScalarUInt32, 3>, ScalarVector3u>();
        pybind11_type_alias<ek::Array<ScalarUInt32, 3>, ScalarPoint3u>();

        pybind11_type_alias<ek::Array<ScalarFloat, 4>,  ScalarVector4f>();
        pybind11_type_alias<ek::Array<ScalarFloat, 4>,  ScalarPoint4f>();
        pybind11_type_alias<ek::Array<ScalarInt32, 4>,  ScalarVector4i>();
        pybind11_type_alias<ek::Array<ScalarInt32, 4>,  ScalarPoint4i>();
        pybind11_type_alias<ek::Array<ScalarUInt32, 4>, ScalarVector4u>();
        pybind11_type_alias<ek::Array<ScalarUInt32, 4>, ScalarPoint4u>();
    }

    if constexpr (is_spectral_v<UnpolarizedSpectrum>)
        pybind11_type_alias<ek::Array<Float, UnpolarizedSpectrum::Size>,
                            UnpolarizedSpectrum>();

    if constexpr (is_polarized_v<Spectrum>)
        pybind11_type_alias<ek::Matrix<ek::Array<Float, UnpolarizedSpectrum::Size>, 4>,
                            Spectrum>();

    if constexpr (ek::is_array_v<Float>)
        pybind11_type_alias<UInt64, ek::replace_scalar_t<Float, const Object *>>();

    m.attr("UnpolarizedSpectrum") = get_type_handle<UnpolarizedSpectrum>();
    m.attr("Spectrum") = get_type_handle<Spectrum>();

    m.attr("float_dtype") = std::is_same_v<ScalarFloat, float> ? "f" : "d";

    m.attr("is_monochromatic") = is_monochromatic_v<Spectrum>;
    m.attr("is_rgb") = is_rgb_v<Spectrum>;
    m.attr("is_spectral") = is_spectral_v<Spectrum>;
    m.attr("is_polarized") = is_polarized_v<Spectrum>;

    m.attr("USE_OPTIX") = ek::is_cuda_array_v<Float>;

    #if defined(MTS_ENABLE_EMBREE)
        m.attr("USE_EMBREE")  = !ek::is_cuda_array_v<Float>;
    #else
        m.attr("USE_EMBREE")  = false;
    #endif

    if constexpr (ek::is_cuda_array_v<Float>)
        cie_alloc();

    MTS_PY_IMPORT(Object);
    MTS_PY_IMPORT(Ray);
    MTS_PY_IMPORT(BoundingBox);
    MTS_PY_IMPORT(BoundingSphere);
    MTS_PY_IMPORT(Frame);
    MTS_PY_IMPORT(DiscreteDistribution);
    MTS_PY_IMPORT(DiscreteDistribution2D);
    MTS_PY_IMPORT(ContinuousDistribution);
    MTS_PY_IMPORT(IrregularContinuousDistribution);
    MTS_PY_IMPORT_SUBMODULE(math);
    MTS_PY_IMPORT(qmc);
    MTS_PY_IMPORT(Properties);
    MTS_PY_IMPORT(rfilter);
    MTS_PY_IMPORT(sample_tea);
    // MTS_PY_IMPORT_SUBMODULE(spline); // TODO refactoring
    MTS_PY_IMPORT(Spectrum);
    MTS_PY_IMPORT(Transform);
    MTS_PY_IMPORT(AnimatedTransform);
    MTS_PY_IMPORT(Hierarchical2D);
    MTS_PY_IMPORT(Marginal2D);
    MTS_PY_IMPORT(vector);
    MTS_PY_IMPORT_SUBMODULE(warp);
    MTS_PY_IMPORT_SUBMODULE(xml);

    py::object core_ext = py::module::import("mitsuba.core_ext");
    cast_object = (Caster) (void *)((py::capsule) core_ext.attr("cast_object"));

    // Change module name back to correct value
    m.attr("__name__") = "mitsuba." ENOKI_TOSTRING(MODULE_NAME);
}

#undef CHANGE_SUBMODULE_NAME
#undef CHANGE_BACK_SUBMODULE_NAME
