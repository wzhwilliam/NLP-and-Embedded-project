package WDM.service.Impl;

import WDM.mapper.ItemMapper;
import WDM.service.OrderService;
import WDM.utils.ResponseCode;
import WDM.utils.UniqueOrderGenerate;
import WDM.mapper.OrderMapper;
import WDM.pojo.Item;
import WDM.pojo.Order;
import com.github.yitter.idgen.YitIdHelper;
import WDM.clients.PaymentClient;
import WDM.clients.StockClient;
import WDM.pojo.Stock;
import io.seata.core.context.RootContext;
import io.seata.core.exception.TransactionException;
import io.seata.spring.annotation.GlobalTransactional;
import io.seata.tm.api.GlobalTransactionContext;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

@Slf4j
@Service
public class OrderServiceImpl implements OrderService {
    @Autowired
    private OrderMapper orderMapper;

    @Autowired
    private ItemMapper itemMapper;

    @Autowired
    private StockClient stockClient;

    @Autowired
    private PaymentClient paymentClient;


    /**
     * @param userId
     * @return
     */
    @Override
//    @Transactional
//    @GlobalLock
    public long createOrder(long userId) {
        long orderId = YitIdHelper.nextId();
        if (orderMapper.createOrder(userId, orderId)) {
            return orderId;
        } else {
            return -1;
        }
    }

    /**
     * @param orderId
     * @return
     */
    @Override
    @Transactional
    public Boolean removeOrder(long orderId) {
        return orderMapper.removeOrder(orderId);
    }

    /**
     * @param orderId
     * @return
     */
//    @GlobalLock
    @Override
//    @Transactional
    public Order findOrder(long orderId) {
        Order order = orderMapper.findOrder(orderId);
        List<Item> items = (itemMapper.findItem(orderId));
        List<Long> itemIds = new LinkedList<>();
        for (Item item : items) {
            itemIds.add(item.getItemId());
        }
        order.setItems(itemIds);
        double cost = 0;
        for (Item item : items) {
            cost += (item.getAmount() * item.getPrice());
        }
        order.setTotalCost(cost);
        return order;
    }

    /**
     * @param orderId
     * @param itemId
     * @return
     */
    @Override
    @Transactional
    public Boolean addItem(long orderId, long itemId) {
        for (Item item : itemMapper.findItem(orderId)) {
            if (item.getItemId() == itemId) {
                itemMapper.updateAmount(itemId);
                return true;
            }
        }
        Stock stock = stockClient.findPrice(itemId);
        UniqueOrderGenerate idWorker = new UniqueOrderGenerate(0, 0);
        long id = idWorker.nextId();
        itemMapper.addItem(id, orderId, itemId, stock.getPrice());
        return true;
    }

    /**
     * @param orderId
     * @param itemId
     * @return
     */
    @Override
    @Transactional
    public Boolean removeItem(long orderId, long itemId) {
        return itemMapper.removeItem(orderId, itemId);
    }

    /**
     * @param order
     * @return
     */
    Lock lock = new ReentrantLock();

    @Override
    @GlobalTransactional(rollbackFor = Exception.class)
    public Boolean checkout(Order order) throws TransactionException {

        try {
//            lock.lock();
//            String XID = RootContext.getXID();
            log.info("Seata global transaction id{}", RootContext.getXID());
            long itime = System.currentTimeMillis();
            List<Item> items = itemMapper.findItem(order.getOrderId());
            long mitime = System.currentTimeMillis();
            log.info("findItem time=================>{}", mitime - itime);
//            for (Item item : items) {
//                if (stockClient.findStock(item.getItemId())<item.getAmount()) {
//                    throw new TransactionException("stock not enough");
//                }
//            }
//            if (paymentClient.getCredit(order.getUserId()).getCredit()<order.getTotalCost()) {
//                throw new TransactionException("Credit not enough");
//            }
            long stime = System.currentTimeMillis();
            for (Item item : items) {
                if (stockClient.subtract(item.getItemId(), item.getAmount()).equals(new ResponseCode().error())) {
                    throw new TransactionException("stock failed");
                }
            }
            long etime = System.currentTimeMillis();
            log.info("Stock time=================>{}", etime - stime);
            if (paymentClient.pay(order.getUserId(), order.getOrderId(), order.getTotalCost()).equals(new ResponseCode().error())) {
                throw new TransactionException("payment failed");
            }
            long ptime = System.currentTimeMillis();
            log.info("Payment time=================>{}", ptime - etime);
            orderMapper.checkout(order.getOrderId());
            long qtime = System.currentTimeMillis();
            log.info("check time=================>{}", qtime - ptime);
//            GlobalTransactionContext.reload(RootContext.getXID()).commit();
            return true;
        } catch (Exception e) {
            log.error("checkout failed:{}", e.getMessage(), e);
            log.info("Seata global transaction id=================>{}", RootContext.getXID());

            long atime = System.currentTimeMillis();
            GlobalTransactionContext.reload(RootContext.getXID()).rollback();
            long btime = System.currentTimeMillis();
            log.info("rollback time=================>{}", btime - atime);
            return false;
        } finally {
//            lock.unlock();
        }
    }

    /**
     * @param orderId
     * @return
     */
//    @Transactional
    @Override
    public void cancelOrder(long orderId) {
        orderMapper.cancelOrder(orderId);
    }
}
