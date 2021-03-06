using UnityEngine;

namespace UTJ.Alembic
{
    public class AlembicXform : AlembicElement
    {
        aiXform m_abcSchema;
        aiXformData m_abcData;

        public override void AbcSetup(aiObject abcObj, aiSchema abcSchema)
        {
            base.AbcSetup(abcObj, abcSchema);

            m_abcSchema = (aiXform)abcSchema;
        }

        public override void AbcSyncDataEnd()
        {
            if (!m_abcSchema.schema.isDataUpdated)
                return;

            m_abcSchema.sample.GetData(ref m_abcData);
            if (m_abcData.inherits)
            {
                abcTreeNode.linkedGameObj.transform.localPosition = m_abcData.translation;
                abcTreeNode.linkedGameObj.transform.localRotation = m_abcData.rotation;
                abcTreeNode.linkedGameObj.transform.localScale = m_abcData.scale;
            }
            else
            {
                abcTreeNode.linkedGameObj.transform.position = m_abcData.translation;
                abcTreeNode.linkedGameObj.transform.rotation = m_abcData.rotation;
                abcTreeNode.linkedGameObj.transform.localScale = m_abcData.scale;
            }
        }
    }
}
