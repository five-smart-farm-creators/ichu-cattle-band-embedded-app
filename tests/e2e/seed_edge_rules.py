"""Write the monitoring rules and collar assignment that the Edge gets only from the ICHU cloud.

CI runs no cloud, so the collar end-to-end job seeds them through the Edge repositories instead.
"""

import uuid

from app import create_app
from monitoring.domain.entities import MonitoringRuleSet
from monitoring.domain.value_objects import (
    AlertSeverity,
    DeviceAssignment,
    MonitoredMetric,
    Threshold,
)
from monitoring.infrastructure.repositories import (
    DeviceAssignmentRepository,
    MonitoringRuleSetRepository,
)

COLLAR_ID = "9d4a5f3b-3a6e-4f84-8daa-4e5f6a7b8c93"
LIFE_STAGE = "MilkingCow"
REVISION = 1


def main() -> None:
    # app.py also runs create_app() on import; every step in it is idempotent.
    create_app()
    # The Critical range contains the Warning range, so the worst breached rule sets the severity.
    thresholds = (
        Threshold(
            uuid.uuid4(), LIFE_STAGE, MonitoredMetric.BODY_TEMPERATURE, 38.0, 39.5, AlertSeverity.WARNING
        ),
        Threshold(
            uuid.uuid4(), LIFE_STAGE, MonitoredMetric.BODY_TEMPERATURE, 37.0, 40.0, AlertSeverity.CRITICAL
        ),
    )
    MonitoringRuleSetRepository().replace(MonitoringRuleSet(uuid.uuid4(), REVISION, thresholds, ()))
    DeviceAssignmentRepository().save(DeviceAssignment(COLLAR_ID, uuid.uuid4(), LIFE_STAGE))
    print(f"Seeded rule revision {REVISION} for {LIFE_STAGE} and assigned collar {COLLAR_ID}")


if __name__ == "__main__":
    main()
