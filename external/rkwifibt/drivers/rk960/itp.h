/*
 * Copyright (c) 2022, Fuzhou Rockchip Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef RK960_ITP_H_INCLUDED
#define RK960_ITP_H_INCLUDED

struct cw200_common;
struct wsm_tx_confirm;
struct dentry;

#ifdef CONFIG_RK960_ITP

/*extern*/ struct ieee80211_channel;

#define TEST_MODE_NO_TEST	(0)
#define TEST_MODE_RX_TEST	(1)
#define TEST_MODE_TX_TEST	(2)

#define itp_printk(...) printk(__VA_ARGS__)
#define ITP_DEFAULT_DA_ADDR {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}
#define ITP_MIN_DATA_SIZE 6
#define ITP_MAX_DATA_SIZE 1600
#define ITP_TIME_THRES_US 10000
#define ITP_US_TO_MS(x) ((x)/1000)
#define ITP_MS_TO_US(x) ((x)*1000)
#if ((ITP_US_TO_MS(ITP_TIME_THRES_US))*HZ/1000) < 1
#warning not able to achieve non-busywaiting ITP_TIME_THRES_US\
precision with current HZ value !
#endif
#define ITP_BUF_SIZE 255

enum rk960_itp_data_modes {
	ITP_DATA_ZEROS,
	ITP_DATA_ONES,
	ITP_DATA_ZERONES,
	ITP_DATA_RANDOM,
	ITP_DATA_MAX_MODE,
};

enum rk960_itp_version_type {
	ITP_CHIP_ID,
	ITP_FW_VER,
};

enum rk960_itp_preamble_type {
	ITP_PREAMBLE_LONG,
	ITP_PREAMBLE_SHORT,
	ITP_PREAMBLE_OFDM,
	ITP_PREAMBLE_MIXED,
	ITP_PREAMBLE_GREENFIELD,
	ITP_PREAMBLE_MAX,
};

struct rk960_itp {
	struct rk960_common *priv;
	atomic_t open_count;
	atomic_t awaiting_confirm;
	struct sk_buff_head log_queue;
	wait_queue_head_t read_wait;
	wait_queue_head_t write_wait;
	wait_queue_head_t close_wait;
	struct ieee80211_channel *saved_channel;
	atomic_t stop_tx;
	struct delayed_work tx_work;
	struct delayed_work tx_finish;
	spinlock_t tx_lock;
	struct timespec last_sent;
	atomic_t test_mode;
	int rx_cnt;
	long rx_rssi;
	int rx_rssi_max;
	int rx_rssi_min;
	unsigned band;
	unsigned ch;
	unsigned rate;
	unsigned preamble;
	unsigned int number;
	unsigned data_mode;
	int interval_us;
	int power;
	u8 *data;
	int hdr_len;
	int data_len;
	int id;
};

int rk960_itp_init(struct rk960_common *priv);
void rk960_itp_release(struct rk960_common *priv);

bool rk960_is_itp(struct rk960_common *priv);
bool rk960_itp_rxed(struct rk960_common *priv, struct sk_buff *skb);
void rk960_itp_wake_up_tx(struct rk960_common *priv);
int rk960_itp_get_tx(struct rk960_common *priv, u8 ** data,
		     size_t * tx_len, int *burst);
bool rk960_itp_tx_running(struct rk960_common *priv);

#else /* CONFIG_RK960_ITP */

static inline int rk960_itp_init(struct rk960_common *priv)
{
	return 0;
}

static inline void rk960_itp_release(struct rk960_common *priv)
{
}

static inline bool rk960_is_itp(struct rk960_common *priv)
{
	return false;
}

static inline bool rk960_itp_rxed(struct rk960_common *priv,
				  struct sk_buff *skb)
{
	return false;
}

static inline void rk960_itp_consume_txed(struct rk960_common *priv)
{
}

static inline void rk960_itp_wake_up_tx(struct rk960_common *priv)
{
}

static inline int rk960_itp_get_tx(struct rk960_common *priv, u8 ** data,
				   size_t * tx_len, int *burst)
{
	return 0;
}

static inline bool rk960_itp_tx_running(struct rk960_common *priv)
{
	return false;
}

#endif /* CONFIG_RK960_ITP */

#endif /* RK960_ITP_H_INCLUDED */
