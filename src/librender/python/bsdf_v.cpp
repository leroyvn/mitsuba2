#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/shape.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/python/python.h>

MTS_PY_EXPORT(BSDFSample) {
    MTS_PY_IMPORT_TYPES()

    m.def("has_flag", [](UInt32 flags, BSDFFlags f) { return has_flag(flags, f); });

    auto bs = py::class_<BSDFSample3f>(m, "BSDFSample3f", D(BSDFSample3))
        .def(py::init<>())
        .def(py::init<const Vector3f &>(), "wo"_a, D(BSDFSample3, BSDFSample3))
        .def(py::init<const BSDFSample3f &>(), "bs"_a, "Copy constructor")
        .def_readwrite("wo", &BSDFSample3f::wo, D(BSDFSample3, wo))
        .def_readwrite("pdf", &BSDFSample3f::pdf, D(BSDFSample3, pdf))
        .def_readwrite("eta", &BSDFSample3f::eta, D(BSDFSample3, eta))
        .def_readwrite("sampled_type", &BSDFSample3f::sampled_type, D(BSDFSample3, sampled_type))
        .def_readwrite("sampled_component", &BSDFSample3f::sampled_component, D(BSDFSample3, sampled_component))
        .def_repr(BSDFSample3f);

    MTS_PY_ENOKI_STRUCT(bs, BSDFSample3f, wo, pdf, eta, sampled_type, sampled_component);
}

/// Trampoline for derived types implemented in Python
MTS_VARIANT class PyBSDF : public BSDF<Float, Spectrum> {
public:
    MTS_IMPORT_TYPES(BSDF)

    PyBSDF(const Properties &props) : BSDF(props) { }

    std::pair<BSDFSample3f, Spectrum>
    sample(const BSDFContext &ctx, const SurfaceInteraction3f &si,
           Float sample1, const Point2f &sample2,
           Mask active) const override {
        using Return = std::pair<BSDFSample3f, Spectrum>;
        PYBIND11_OVERRIDE_PURE(Return, BSDF, sample, ctx, si, sample1, sample2, active);
    }

    Spectrum eval(const BSDFContext &ctx,
                  const SurfaceInteraction3f &si,
                  const Vector3f &wo,
                  Mask active) const override {
        PYBIND11_OVERRIDE_PURE(Spectrum, BSDF, eval, ctx, si, wo, active);
    }

    Float pdf(const BSDFContext &ctx,
              const SurfaceInteraction3f &si,
              const Vector3f &wo,
              Mask active) const override {
        PYBIND11_OVERRIDE_PURE(Float, BSDF, pdf, ctx, si, wo, active);
    }

    std::pair<Spectrum, Float> eval_pdf(const BSDFContext &ctx,
              const SurfaceInteraction3f &si,
              const Vector3f &wo,
              Mask active) const override {
        using Return = std::pair<Spectrum, Float>;
        PYBIND11_OVERRIDE_PURE(Return, BSDF, eval_pdf, ctx, si, wo, active);
    }

    std::string to_string() const override {
        PYBIND11_OVERRIDE_PURE(std::string, BSDF, to_string,);
    }

    using BSDF::m_flags;
    using BSDF::m_components;
};

template <typename Ptr, typename Cls> void bind_bsdf_generic(Cls &cls) {
    MTS_PY_IMPORT_TYPES()

    cls.def("sample",
            [](Ptr bsdf, const BSDFContext &ctx, const SurfaceInteraction3f &si,
               Float sample1, const Point2f &sample2, Mask active) {
                return bsdf->sample(ctx, si, sample1, sample2, active);
            }, "ctx"_a, "si"_a, "sample1"_a, "sample2"_a,
            "active"_a = true, D(BSDF, sample))
        .def("eval",
             [](Ptr bsdf, const BSDFContext &ctx, const SurfaceInteraction3f &si,
                const Vector3f &wo,
                Mask active) { return bsdf->eval(ctx, si, wo, active);
             }, "ctx"_a, "si"_a, "wo"_a, "active"_a = true, D(BSDF, eval))
        .def("pdf",
             [](Ptr bsdf, const BSDFContext &ctx, const SurfaceInteraction3f &si,
                const Vector3f &wo,
                Mask active) { return bsdf->pdf(ctx, si, wo, active);
             }, "ctx"_a, "si"_a, "wo"_a, "active"_a = true, D(BSDF, pdf))
        .def("eval_pdf",
             [](Ptr bsdf, const BSDFContext &ctx, const SurfaceInteraction3f &si,
                const Vector3f &wo,
                Mask active) { return bsdf->eval_pdf(ctx, si, wo, active);
             }, "ctx"_a, "si"_a, "wo"_a, "active"_a = true, D(BSDF, eval_pdf))
        .def("eval_null_transmission",
             [](Ptr bsdf, const SurfaceInteraction3f &si, Mask active) {
                 return bsdf->eval_null_transmission(si, active);
             }, "si"_a, "active"_a = true, D(BSDF, eval_null_transmission))
        .def("flags", [](Ptr bsdf) { return bsdf->flags(); }, D(BSDF, flags))
        .def("needs_differentials",
             [](Ptr bsdf) { return bsdf->needs_differentials(); },
             D(BSDF, needs_differentials));

    if constexpr (ek::is_array_v<Ptr>)
        bind_enoki_ptr_array(cls);
}

MTS_PY_EXPORT(BSDF) {
    MTS_PY_IMPORT_TYPES(BSDF, BSDFPtr)
    using PyBSDF = PyBSDF<Float, Spectrum>;

    auto bsdf = py::class_<BSDF, PyBSDF, Object, ref<BSDF>>(m, "BSDF", D(BSDF))
        .def(py::init<const Properties&>(), "props"_a)
        .def("flags", py::overload_cast<size_t, Mask>(&BSDF::flags, py::const_),
            "index"_a, "active"_a = true, D(BSDF, flags, 2))
        .def_method(BSDF, component_count, "active"_a = true)
        .def_method(BSDF, id)
        .def_readwrite("m_flags",      &PyBSDF::m_flags)
        .def_readwrite("m_components", &PyBSDF::m_components)
        .def("__repr__", &BSDF::to_string);

    bind_bsdf_generic<BSDF *>(bsdf);

    if constexpr (ek::is_array_v<BSDFPtr>) {
        py::object ek       = py::module_::import("enoki"),
                   ek_array = ek.attr("ArrayBase");

        py::class_<BSDFPtr> cls(m, "BSDFPtr", ek_array);
        bind_bsdf_generic<BSDFPtr>(cls);
        pybind11_type_alias<UInt32, ek::replace_scalar_t<UInt32, BSDFFlags>>();
    }

    MTS_PY_REGISTER_OBJECT("register_bsdf", BSDF)
}
