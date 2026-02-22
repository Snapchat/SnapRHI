#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/common/Logging.hpp"

#include "snap/rhi/common/Throw.h"
#include <array>
#include <cstdio>
#include <string>

namespace {
std::string buildValidationTagStr(const snap::rhi::ValidationTag tags) {
    if (static_cast<uint64_t>(tags) == 0) {
        return "ValidationTag::None";
    }

    const std::string result = "ValidationTag::{" + snap::rhi::convertToString(tags) + "}";
    return result;
}
} // unnamed namespace

namespace snap::rhi::backend::common {
ValidationLayer::ValidationLayer(const snap::rhi::ValidationTag tags, const snap::rhi::ReportLevel reportLevel)
    : enabledTags(tags), reportLevel(reportLevel) {}

void ValidationLayer::reportImpl(const snap::rhi::ReportLevel messageReportLevel,
                                 const snap::rhi::ValidationTag messageValidationTags,
                                 snap::rhi::common::zstring_view message) const {
    const std::string tagsStr = buildValidationTagStr(messageValidationTags);
    SNAP_RHI_LOGI("[ReportLevel::%s][%s] message: %s",
                  getReportLevelStr(messageReportLevel).data(),
                  tagsStr.data(),
                  message.data());

    if (messageReportLevel >= snap::rhi::ReportLevel::Error) {
        SNAP_RHI_LOGE("[%s] message: %s", tagsStr.data(), message.data());
        if (!((messageReportLevel == snap::rhi::ReportLevel::Error) &&
              ((messageValidationTags & snap::rhi::ValidationTag::DestroyOp) == snap::rhi::ValidationTag::DestroyOp))) {
            snap::rhi::common::throwException(message.data());
        }
    }
}
} // namespace snap::rhi::backend::common
