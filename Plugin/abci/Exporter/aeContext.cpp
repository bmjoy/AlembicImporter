#include "pch.h"
#include "aeInternal.h"
#include "aeContext.h"
#include "aeObject.h"

aeContext::aeContext()
{
    abciDebugLog("aeContext::aeContext()");
}

aeContext::~aeContext()
{
    abciDebugLog("aeContext::~aeContext()");
    reset();
}

void aeContext::reset()
{
    waitAsync();

    if (m_archive != nullptr) {
        if (m_config.time_sampling_type == aeTimeSamplingType::Uniform) {
            // start time in default time sampling maybe changed.
            auto ts = Abc::TimeSampling(abcChrono(1.0f / m_config.frame_rate), m_config.start_time);
            *m_archive.getTimeSampling(1) = ts;
        }
        else if (m_config.time_sampling_type == aeTimeSamplingType::Cyclic) {
            for (int i = 1; i < (int)m_timesamplings.size(); ++i) {
                auto &t = m_timesamplings[i];
                if (!t.times.empty()) {
                    auto ts = Abc::TimeSampling(Abc::TimeSamplingType((uint32_t)t.times.size(), t.times.back()), t.times);
                    *m_archive.getTimeSampling(i) = ts;
                }
            }
        }
        else if (m_config.time_sampling_type == aeTimeSamplingType::Acyclic) {
            for (int i = 1; i < (int)m_timesamplings.size(); ++i) {
                auto &t = m_timesamplings[i];
                auto ts = Abc::TimeSampling(Abc::TimeSamplingType(Abc::TimeSamplingType::kAcyclic), t.times);
                *m_archive.getTimeSampling(i) = ts;
            }
        }
    }

    m_node_top.reset();
    m_timesamplings.clear();
    m_archive.reset(); // flush archive
}

void aeContext::setConfig(const aeConfig &conf)
{
    m_config = conf;
}

bool aeContext::openArchive(const char *path)
{
    reset();

    abciDebugLog("aeContext::openArchive() %s", path);
    try {
        if (m_config.archive_type == aeArchiveType::HDF5) {
            m_archive = Abc::OArchive(Alembic::AbcCoreHDF5::WriteArchive(), path);
        }
        else if (m_config.archive_type == aeArchiveType::Ogawa) {
            m_archive = Abc::OArchive(Alembic::AbcCoreOgawa::WriteArchive(), path);
        }
        else {
            return false;
        }
    }
    catch (Alembic::Util::Exception e) {
        abciDebugLog("Failed (%s)", e.what());
        return false;
    }

    // reserve 'default' time sampling. update it later in reset()
    auto tsi = addTimeSampling(m_config.start_time);

    auto *top = new AbcGeom::OObject(m_archive, AbcGeom::kTop, tsi);
    m_node_top.reset(new aeObject(this, nullptr, top, tsi));
    return true;
}

const aeConfig& aeContext::getConfig() const
{
    return m_config;
}

aeObject* aeContext::getTopObject()
{
    return m_node_top.get();
}

uint32_t aeContext::getNumTimeSampling() const
{
    return (uint32_t)m_timesamplings.size();
}

aeTimeSampling& aeContext::getTimeSampling(uint32_t i)
{
    return m_timesamplings[i];
}

uint32_t aeContext::addTimeSampling(float start_time)
{
    auto ts = Abc::TimeSampling(abcChrono(1.0f / m_config.frame_rate), abcChrono(start_time));
    auto tsi = m_archive.addTimeSampling(ts);
    m_timesamplings.resize(tsi + 1);
    return tsi;
}

void aeContext::addTime(float time, uint32_t tsi)
{
    // if tsi==-1, add time to all time samplings
    if (tsi == -1) {
        for (size_t i = 1; i < m_timesamplings.size(); ++i) {
            auto &ts = m_timesamplings[i];
            if (ts.times.empty() || ts.times.back() != time) {
                ts.times.push_back(time);
            }
        }
    }
    else {
        auto &ts = m_timesamplings[tsi];
        if (ts.times.empty() || ts.times.back() != time) {
            ts.times.push_back(time);
        }
    }
}

void aeContext::markFrameBegin()
{
    waitAsync();
}

void aeContext::markFrameEnd()
{
    // kick async tasks
    m_async_task_future = std::async(std::launch::async, [this]() {
        for (auto& task : m_async_tasks)
            task();
        m_async_tasks.clear();
    });
}

void aeContext::addAsync(const std::function<void()>& task)
{
    m_async_tasks.push_back(task);
}

void aeContext::waitAsync()
{
    if (m_async_task_future.valid()) {
        m_async_task_future.wait();
    }
}
