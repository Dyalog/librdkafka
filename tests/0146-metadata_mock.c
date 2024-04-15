/*
 * librdkafka - Apache Kafka C library
 *
 * Copyright (c) 2024, Confluent Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "test.h"


/**
 * @brief Metadata should persists in cache after
 *        a full metadata refresh.
 *
 * @param assignor Assignor to use
 */
static void do_test_metadata_persists_in_cache(const char *assignor) {
        rd_kafka_t *rk;
        const char *bootstraps;
        rd_kafka_mock_cluster_t *mcluster;
        const char *topic = test_mk_topic_name(__FUNCTION__, 1);
        rd_kafka_conf_t *conf;
        rd_kafka_topic_t *rkt;
        const rd_kafka_metadata_t *md;
        rd_kafka_topic_partition_list_t *subscription;

        SUB_TEST_QUICK("%s", assignor);

        mcluster = test_mock_cluster_new(3, &bootstraps);
        rd_kafka_mock_topic_create(mcluster, topic, 1, 1);

        test_conf_init(&conf, NULL, 10);
        test_conf_set(conf, "bootstrap.servers", bootstraps);
        test_conf_set(conf, "partition.assignment.strategy", assignor);
        test_conf_set(conf, "group.id", topic);

        rk = test_create_handle(RD_KAFKA_CONSUMER, conf);

        subscription = rd_kafka_topic_partition_list_new(1);
        rd_kafka_topic_partition_list_add(subscription, topic, 0);

        rkt = test_create_consumer_topic(rk, topic);

        /* Metadata for topic is available */
        TEST_CALL_ERR__(rd_kafka_metadata(rk, 0, rkt, &md, 1000));
        rd_kafka_metadata_destroy(md);
        md = NULL;

        /* Subscribe to same topic */
        TEST_CALL_ERR__(rd_kafka_subscribe(rk, subscription));

        /* Request full metadata */
        TEST_CALL_ERR__(rd_kafka_metadata(rk, 1, NULL, &md, 1000));
        rd_kafka_metadata_destroy(md);
        md = NULL;

        /* Subscribing shouldn't give UNKNOWN_TOPIC_OR_PART err.
         * Verify no error was returned. */
        test_consumer_poll_no_msgs("no error", rk, 0, 100);

        rd_kafka_topic_partition_list_destroy(subscription);
        rd_kafka_topic_destroy(rkt);
        rd_kafka_destroy(rk);
        test_mock_cluster_destroy(mcluster);

        SUB_TEST_PASS();
}

int main_0146_metadata_mock(int argc, char **argv) {
        TEST_SKIP_MOCK_CLUSTER(0);

        do_test_metadata_persists_in_cache("range");

        do_test_metadata_persists_in_cache("cooperative-sticky");

        return 0;
}