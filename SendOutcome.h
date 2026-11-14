#ifndef SEND_OUTCOME_H
#define SEND_OUTCOME_H

// Delivered and Rejected both remove the reading from the buffer; Retry keeps it.
enum class SendOutcome { Delivered, Rejected, Retry };

#endif  // SEND_OUTCOME_H
