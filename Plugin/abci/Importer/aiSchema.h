#pragma once


class aiSample
{
public:
    aiSample(aiSchema *schema);
    virtual ~aiSample();

    virtual aiSchema* getSchema() const { return m_schema; }
    const aiConfig& getConfig() const;

    virtual void waitAsync() {}
    void markForceSync();

protected:
    aiSchema *m_schema = nullptr;
    bool m_force_sync = false;
};


class aiSchema : public aiObject
{
using super = aiObject;
public:
    using aiPropertyPtr = std::unique_ptr<aiProperty>;

    aiSchema(aiObject *parent, const abcObject &abc);
    virtual ~aiSchema();

    bool isConstant() const;
    bool isDataUpdated() const;
    void markForceUpdate();
    void markForceSync();
    int getNumProperties() const;
    aiProperty* getPropertyByIndex(int i);
    aiProperty* getPropertyByName(const std::string& name);

protected:
    virtual abcProperties getAbcProperties() = 0;
    void setupProperties();
    void updateProperties(const abcSampleSelector& ss);

protected:
    bool m_constant = false;
    bool m_data_updated = false;
    bool m_force_update = false;
    bool m_force_sync = false;
    std::vector<aiPropertyPtr> m_properties; // sorted vector
};


template <class Traits>
class aiTSchema : public aiSchema
{
using super = aiSchema;
public:
    using Sample = typename Traits::SampleT;
    using SamplePtr = std::shared_ptr<Sample>;
    using SampleCont = std::map<int64_t, SamplePtr>;
    using AbcSchema = typename Traits::AbcSchemaT;
    using AbcSchemaObject = Abc::ISchemaObject<AbcSchema>;


    aiTSchema(aiObject *parent, const abcObject &abc)
        : super(parent, abc)
    {
        AbcSchemaObject abcObj(abc, Abc::kWrapExisting);
        m_schema = abcObj.getSchema();
        m_constant = m_schema.isConstant();
        m_time_sampling = m_schema.getTimeSampling();
        m_num_samples = static_cast<int64_t>(m_schema.getNumSamples());
        setupProperties();
    }

    int getTimeSamplingIndex() const
    {
        return getContext()->getTimeSamplingIndex(m_schema.getTimeSampling());
    }

    int getSampleIndex(const abcSampleSelector& ss) const
    {
        return static_cast<int>(ss.getIndex(m_time_sampling, m_num_samples));
    }

    float getSampleTime(const abcSampleSelector& ss) const
    {
        return static_cast<float>(m_time_sampling->getSampleTime(ss.getRequestedIndex()));
    }

    int getNumSamples() const
    {
        return static_cast<int>(m_num_samples);
    }

    void updateSample(const abcSampleSelector& ss) override
    {
        if (!m_enabled)
            return;

        Sample* sample = nullptr;
        int64_t sample_index = getSampleIndex(ss);
        auto& config = getConfig();

        if (!m_sample || (!m_constant && sample_index != m_last_sample_index)) {
            m_sample_index_changed = true;
            if (!m_sample)
                m_sample.reset(newSample());
            sample = m_sample.get();
            readSample(*sample, sample_index);
        }
        else {
            m_sample_index_changed = false;
            sample = m_sample.get();
            if ((m_constant || !config.interpolate_samples) && !m_force_update)
                sample = nullptr;
        }

        if (sample && config.interpolate_samples) {
            auto& ts = *m_time_sampling;
            double requested_time = ss.getRequestedTime();
            double index_time = ts.getSampleTime(sample_index);
            double interval = 0;
            if (ts.getTimeSamplingType().isAcyclic()) {
                auto tsi = std::min((size_t)sample_index + 1, ts.getNumStoredTimes() - 1);
                interval = ts.getSampleTime(tsi) - index_time;
            }
            else {
                interval = ts.getTimeSamplingType().getTimePerCycle();
            }

            float prev_offset = m_current_time_offset;
            m_current_time_offset = interval == 0.0 ? 0.0f :
                (float)std::max(0.0, std::min((requested_time - index_time) / interval, 1.0));
            m_current_time_interval = (float)interval;

            // skip if time offset is not changed
            if (sample_index == m_last_sample_index && prev_offset == m_current_time_offset && !m_force_update)
                sample = nullptr;
        }

        if (sample) {
            if (m_force_sync)
                sample->markForceSync();

            cookSample(*sample);
            m_data_updated = true;
        }
        else {
            m_data_updated = false;
        }
        updateProperties(ss);

        m_last_sample_index = sample_index;
        m_force_update = false;
        m_force_sync = false;
    }

    Sample* getSample() override
    {
        return m_sample.get();
    }

protected:
    virtual Sample* newSample() = 0;
    virtual void readSample(Sample& sample, uint64_t idx) = 0;
    virtual void cookSample(Sample& sample) {}

    AbcGeom::ICompoundProperty getAbcProperties() override
    {
        return m_schema.getUserProperties();
    }

protected:
    AbcSchema m_schema;
    Abc::TimeSamplingPtr m_time_sampling;
    SamplePtr m_sample;
    int64_t m_num_samples = 0;
    int64_t m_last_sample_index = -1;
    float m_current_time_offset = 0;
    float m_current_time_interval = 0;
    bool m_sample_index_changed = false;
};
