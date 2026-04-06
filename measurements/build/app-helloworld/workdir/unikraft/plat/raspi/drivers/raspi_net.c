#include <errno.h>
#include <uk/init.h>
#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/print.h>
#include <uk/netdev.h>
#include <uk/netdev_core.h>
#include <uk/netdev_driver.h>
#include <uk/ring.h>
#include <uk/sched.h>
#include <uk/thread.h>
#include <string.h>

#define DRIVER_NAME	"raspi-net"
#define RASPI_NET_MAX_MTU 1500
#define RASPI_PKT_BUFFER_ALIGN 2048 // Might not need this or it can be different but it was currently just taken from `VIRTIO_PKT_BUFFER_ALIGN` to avoid petintial virtual memory issues?
#define RASPI_MAX_QUEUE_PAIRS 1
#define RASPI_MAX_N_DESCRIPTORS 2048 // TODO: This was kind of chosen randomly. In Linux, you can find this value by inspecting the virtio device's configuration. This can be done by reading the /sys filesystem, specifically the /sys/class/net/<device>/queues/tx-<queue>/tx_max_batch file, where <device> is the name of your network device and <queue> is the number of the queue you're interested in. Or maybe try ethtool.

typedef enum {
	RNET_RX,
	RNET_TX,
} raspiq_type_t;

// TODO: These queues and rings are currently not really used but would be usefull if a non polling version of this device would ever be implemented.
/**
 * @internal structure to represent the transmit queue.
 */
struct raspi_netdev_tx_queue {
	/* The nr. of descriptor user configured */
	__u16 nb_desc;
	/* The underlying ring used for the packet queue */
	struct uk_ring *rq;
	/* The flag to interrupt on the transmit queue */
	__u8 intr_enabled;
	/* Reference to the uk_netdev */
	struct uk_netdev *ndev;
	/* The scatter list and its associated fragements */
	// struct uk_sglist sg;
	// struct uk_sglist_seg sgsegs[NET_MAX_FRAGMENTS];
};

/**
 * @internal structure to represent the receive queue.
 */
struct raspi_netdev_rx_queue {
	/* User-provided receive buffer allocation function */
	uk_netdev_alloc_rxpkts alloc_rxpkts;
	void *alloc_rxpkts_argp;

	/* The nr. of descriptor user configured */
	__u16 nb_desc;
	/* The underlying ring used for the packet queue */
	struct uk_ring *rq;
	/* The flag to interrupt on the transmit queue */
	__u8 intr_enabled;
	/* Reference to the uk_netdev */
	struct uk_netdev *ndev;
	/* The scatter list and its associated fragements */
	// struct uk_sglist sg;
	// struct uk_sglist_seg sgsegs[NET_MAX_FRAGMENTS];
};

struct raspi_net_device {
	// struct virtio_dev *vdev;
	// /* List of all the virtqueue in the pci device */
	// struct virtqueue *vq;
	struct uk_netdev netdev;
	// /* Count of the number of the virtqueues */
	// __u16 max_vqueue_pairs;
	// /* List of the Rx/Tx queue */
	// __u16    rx_vqueue_cnt;
	struct   raspi_netdev_rx_queue rxqs[RASPI_MAX_QUEUE_PAIRS];
	// __u16    tx_vqueue_cnt;
	struct   raspi_netdev_tx_queue txqs[RASPI_MAX_QUEUE_PAIRS];
	/* The netdevice identifier */
	__u16 uid;
	// /* The max mtu */
	// __u16 max_mtu;
	// /* The mtu */
	// __u16 mtu;
	/* The hw address of the netdevice */
	struct uk_hwaddr hw_addr;
	// /*  Netdev state */
	// __u8 state;
	// /* RX promiscuous mode. */
	// __u8 promisc : 1;
};

#define to_raspinetdev(ndev) \
	__containerof(ndev, struct raspi_net_device, netdev)

/**
 * Static global constants
 */
static const char *drv_name = DRIVER_NAME;
static struct uk_alloc *a;

static int rasp_net_drv_init(struct uk_alloc *drv_allocator)
{
	/* driver initialization */
	if (!drv_allocator)
		return -EINVAL;

	a = drv_allocator;
	return 0;
}

static int raspi_netdev_recv(struct uk_netdev *dev,
			      struct raspi_netdev_rx_queue *rxq,
			      struct uk_netbuf **pkt)
{
	unsigned char Buffer[1600] = {0x0};
	unsigned nFrameLength;
	if (!USPiReceiveFrame (Buffer, &nFrameLength))
	{
		return 0;
	}

	UK_ASSERT(nFrameLength <= 1600);

	int cnt = rxq->alloc_rxpkts(rxq->alloc_rxpkts_argp, pkt, 1);
	if (cnt != 1) {
		uk_pr_err("Failed allocate memory for received ethernet frame\n");
		return UK_NETDEV_STATUS_UNDERRUN;
	}

	struct uk_netbuf *netbuf = *pkt;

	UK_ASSERT(netbuf->buflen >= nFrameLength);

	netbuf->data = netbuf->buf;
	netbuf->len = nFrameLength;
	memcpy(netbuf->data, &Buffer, nFrameLength);

	return UK_NETDEV_STATUS_SUCCESS | UK_NETDEV_STATUS_MORE;
}

static int raspi_netdev_xmit(struct uk_netdev *n,
			      struct raspi_netdev_tx_queue *queue,
			      struct uk_netbuf *pkt)
{
	if (!USPiSendFrame ((const void *)pkt->data, pkt->len)) {
		uk_pr_err("Failed to send frame\n");
		return -1;
	}

	uk_netbuf_free(pkt);

	return UK_NETDEV_STATUS_SUCCESS | UK_NETDEV_STATUS_MORE;
}

static const struct uk_hwaddr *raspi_net_mac_get(struct uk_netdev *n)
{
	struct raspi_net_device *d;

	UK_ASSERT(n);
	d = to_raspinetdev(n);
	return &d->hw_addr;
}

static unsigned raspi_net_promisc_get(struct uk_netdev *n __unused)
{
	return 0;
}

static void raspi_net_info_get(struct uk_netdev *dev,
				struct uk_netdev_info *dev_info)
{
	UK_ASSERT(dev && dev_info);

	// Currently only 1 queue pair is supported
	dev_info->max_rx_queues = RASPI_MAX_QUEUE_PAIRS;
	dev_info->max_tx_queues = RASPI_MAX_QUEUE_PAIRS;
	dev_info->in_queue_pairs = RASPI_MAX_QUEUE_PAIRS;
	dev_info->max_mtu = RASPI_NET_MAX_MTU;
	dev_info->ioalign = RASPI_PKT_BUFFER_ALIGN;

	dev_info->nb_encap_tx = 0;
	dev_info->nb_encap_rx = 0;

	dev_info->features = 0x0; // TODO: Check what features we can implement
}

static __u16 raspi_net_mtu_get(struct uk_netdev *n __unused)
{
	return RASPI_NET_MAX_MTU;
}

static int raspi_netdev_txq_info_get(struct uk_netdev *dev,
				      __u16 queue_id,
				      struct uk_netdev_queue_info *qinfo)
{
	int rc = 0;

	UK_ASSERT(dev && qinfo);

	if (unlikely(queue_id >= RASPI_MAX_QUEUE_PAIRS)) {
		uk_pr_err("Invalid queue_id %"__PRIu16"\n", queue_id);
		rc = -EINVAL;
		goto exit;
	}

	qinfo->nb_min = 1;
	qinfo->nb_max = RASPI_MAX_N_DESCRIPTORS;
	qinfo->nb_is_power_of_two = 1;

exit:
	return rc;
}

static int raspi_netdev_rxq_info_get(struct uk_netdev *dev,
				      __u16 queue_id,
				      struct uk_netdev_queue_info *qinfo)
{
	int rc = 0;

	UK_ASSERT(dev && qinfo);
	if (unlikely(queue_id >= RASPI_MAX_QUEUE_PAIRS)) {
		uk_pr_err("Invalid queue id: %"__PRIu16"\n", queue_id);
		rc = -EINVAL;
		goto exit;
	}

	qinfo->nb_min = 1;
	qinfo->nb_max = RASPI_MAX_N_DESCRIPTORS;
	qinfo->nb_is_power_of_two = 1;

exit:
	return rc;
}

/**
 * This function setup the vring infrastructure.
 * @param vndev
 *	Reference to the virtio net device.
 * @param queue_id
 *	User queue identifier
 * @param nr_desc
 *	User configured number of descriptors.
 * @param queue_type
 *	Queue type.
 * @param a
 *	Reference to the allocator.
 */
static int raspi_netdev_queue_setup(struct raspi_net_device *rndev,
		__u16 queue_id, __u16 nr_desc, raspiq_type_t queue_type,
		struct uk_alloc *a)
{
	// TODO: THese queues are currently not used

	int rc = 0;
	int id = 0;
	__u16 max_desc, hwvq_id;
	struct uk_ring *rq;

	if (queue_type == RNET_RX) {
		id = 0;
		max_desc = RASPI_MAX_N_DESCRIPTORS;
	} else {
		id = 0;
		max_desc = RASPI_MAX_N_DESCRIPTORS;
	}

	if (unlikely(max_desc < nr_desc)) {
		uk_pr_err("Max allowed desc: %"__PRIu16" Requested desc:%"__PRIu16"\n",
			  max_desc, nr_desc);
		return -ENOBUFS;
	}

	nr_desc = (nr_desc != 0) ? nr_desc : max_desc;
	uk_pr_debug("Configuring the %d descriptors\n", nr_desc);

	/* Check if the descriptor is a power of 2 */
	if (unlikely(nr_desc & (nr_desc - 1))) {
		uk_pr_err("Expect descriptor count as a power 2\n");
		return -EINVAL;
	}

	rq = uk_ring_alloc(nr_desc, a);
	if (unlikely(PTRISERR(rq))) {
		uk_pr_err("Failed to set up queue %"__PRIu16"\n",
			  queue_id);
		rc = PTR2ERR(rq);
		return rc;
	}

	if (queue_type == RNET_RX) {
		rndev->rxqs[id].ndev = &rndev->netdev;
		rndev->rxqs[id].rq = rq;
		rndev->rxqs[id].nb_desc = nr_desc;
	} else {
		rndev->txqs[id].rq = rq;
		rndev->txqs[id].ndev = &rndev->netdev;
		rndev->txqs[id].nb_desc = nr_desc;
	}
	return id;
}

static struct raspi_netdev_tx_queue *raspi_netdev_tx_queue_setup(
				struct uk_netdev *n, __u16 queue_id,
				__u16 nb_desc __unused,
				struct uk_netdev_txqueue_conf *conf)
{
	struct raspi_netdev_tx_queue *txq = NULL;
	struct raspi_net_device *rndev;
	int rc = 0;

	UK_ASSERT(n);
	rndev = to_raspinetdev(n);
	if (queue_id >= RASPI_MAX_QUEUE_PAIRS) {
		uk_pr_err("Invalid queue identifier: %"__PRIu16"\n",
			  queue_id);
		rc = -EINVAL;
		goto err_exit;
	}
	/* Setup the tx queue */
	rc = raspi_netdev_queue_setup(rndev, queue_id, nb_desc, RNET_TX,
					conf->a);
	if (rc < 0) {
		uk_pr_err("Failed to set up queue %"__PRIu16": %d\n",
			  queue_id, rc);
		goto err_exit;
	}
	txq = &rndev->txqs[rc];
exit:
	return txq;

err_exit:
	txq = ERR2PTR(rc);
	goto exit;
}

static struct raspi_netdev_rx_queue *raspi_netdev_rx_queue_setup(
				struct uk_netdev *n, __u16 queue_id,
				__u16 nb_desc,
				struct uk_netdev_rxqueue_conf *conf)
{
	struct raspi_net_device *rndev;
	struct raspi_netdev_rx_queue *rxq = NULL;
	int rc;

	UK_ASSERT(n);
	UK_ASSERT(conf);
	UK_ASSERT(conf->alloc_rxpkts);

	rndev = to_raspinetdev(n);
	if (queue_id >= RASPI_MAX_QUEUE_PAIRS) {
		uk_pr_err("Invalid queue identifier: %"__PRIu16"\n",
			  queue_id);
		rc = -EINVAL;
		goto err_exit;
	}
	/* Setup the queue with the descriptor */
	rc = raspi_netdev_queue_setup(rndev, queue_id, nb_desc, RNET_RX,
					conf->a);
	if (rc < 0) {
		uk_pr_err("Failed to set up queue %"__PRIu16": %d\n",
			  queue_id, rc);
		goto err_exit;
	}
	rxq  = &rndev->rxqs[rc];

	rxq->alloc_rxpkts = conf->alloc_rxpkts;
	rxq->alloc_rxpkts_argp = conf->alloc_rxpkts_argp;
exit:
	return rxq;

err_exit:
	rxq = ERR2PTR(rc);
	goto exit;
}

static int raspi_netdev_configure(struct uk_netdev *n __unused,
				   const struct uk_netdev_conf *conf __unused)
{
	// If we check the virtio_net implementation we can see that the list of rq and tx queues is allocated here.
	// We only have a single queue for each so it will ba a static array of length 1 at compile time so no need to dynamically
	// allocate aything here.
	return 0;
}

static int raspi_net_start(struct uk_netdev *n)
{
	struct raspi_net_device *d;

	UK_ASSERT(n != NULL);
	d = to_raspinetdev(n);

	if (!USPiEnvInitialize ())
	{
		uk_pr_err("Failed to init USPiEnv.\n");

		return -EIO;
	}
	
	if (!USPiInitialize ())
	{
		uk_pr_err("Cannot initialize USPi\n");

		USPiEnvClose ();
		return -EIO;
	}

	if (!USPiEthernetAvailable ())
	{
		uk_pr_err("Ethernet device not found\n");

		USPiEnvClose ();

		return -ENXIO;
	}

	if (!USPiGetMACAddress (&d->hw_addr.addr_bytes)) {
		uk_pr_err("Failed to get the device hardware address\n");
		return -EIO;
	}

	unsigned nTimeout = 0;
	while (!USPiEthernetIsLinkUp ())
	{
		MsDelay (100);

		if (++nTimeout < 40)
		{
			continue;
		}
		nTimeout = 0;

		uk_pr_err("Link is down\n");
		return -EIO;
	}

	return 0;
}


static const struct uk_netdev_ops raspi_netdev_ops = {
	// TODO: If we enable some of the optional features we might need to use this?
	// .probe = ,
	.configure = raspi_netdev_configure,
	.rxq_configure = raspi_netdev_rx_queue_setup,
	.txq_configure = raspi_netdev_tx_queue_setup,
	.start = raspi_net_start,
	// TODO: If we enable the rx and tx interrupt features (also in raspi_net_info_get) we should actually update these
	// .rxq_intr_enable = ,
	// .rxq_intr_disable = ,
	.info_get = raspi_net_info_get,
	.promiscuous_get = raspi_net_promisc_get,
	.hwaddr_get = raspi_net_mac_get,
	.mtu_get = raspi_net_mtu_get,
	.txq_info_get = raspi_netdev_txq_info_get,
	.rxq_info_get = raspi_netdev_rxq_info_get,
};

static int raspi_net_add_dev()
{
	struct raspi_net_device *rndev;
	int rc = 0;

	rndev = uk_calloc(a, 1, sizeof(*rndev));
	if (!rndev) {
		rc = -ENOMEM;
		goto err_out;
	}

	/* register netdev */
	rndev->netdev.rx_one = raspi_netdev_recv;
	rndev->netdev.tx_one = raspi_netdev_xmit;
	rndev->netdev.ops = &raspi_netdev_ops;

	rc = uk_netdev_drv_register(&rndev->netdev, a, drv_name);
	if (rc < 0) {
		uk_pr_err("Failed to register raspi-net device with libuknet\n");
		goto err_netdev_data;
	}
	rndev->uid = rc;
	rc = 0;

	uk_pr_debug("raspi-net device registered with libuknet\n");

exit:
	return rc;
err_netdev_data:
	uk_free(a, rndev);
err_out:
	goto exit;
}

static int rasp_net_register(struct uk_init_ctx *ictx) {
	int rc;
    uk_pr_err("Registering raspi net dev.\n");
	rc = rasp_net_drv_init(uk_alloc_get_default());
	if (rc < 0) {
		uk_pr_err("Failed to init raspi-net device\n");
		return rc;
	}

	rc = raspi_net_add_dev();
	if (rc < 0) {
		uk_pr_err("Failed to register raspi-net device with libuknet\n");
		return rc;
	}

	return rc;
}

// TODO: I am not sure if USPiReceiveFrame and USPiSendFrame are thread safe? So in the future we should have to share a lock for sending and receiving? Not an issue at this moment.

uk_plat_initcall_prio(rasp_net_register, 0x0, UK_PRIO_EARLIEST);
