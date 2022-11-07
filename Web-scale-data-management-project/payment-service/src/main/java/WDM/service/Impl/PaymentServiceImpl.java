package WDM.service.Impl;

import WDM.mapper.PaymentMapper;
import WDM.pojo.Payment;
import WDM.service.PaymentService;
import com.github.yitter.idgen.YitIdHelper;
import feign.FeignException;
import WDM.pojo.Item;
import WDM.clients.OrderClient;
import WDM.clients.StockClient;
import WDM.pojo.Order;
import io.seata.core.context.RootContext;
import io.seata.core.exception.TransactionException;
import io.seata.spring.annotation.GlobalTransactional;
import io.seata.tm.api.GlobalTransactionContext;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

@Slf4j
@Service
public class PaymentServiceImpl implements PaymentService {

    @Autowired
    PaymentMapper paymentMapper;

    @Autowired
    OrderClient orderClient;

    @Autowired
    StockClient stockClient;

    @Override
//    @Transactional
    public Boolean pay(long userId, double credit) throws FeignException, TransactionException {
        log.info("Seata global transaction id=================>{}", RootContext.getXID());
        RootContext.bind(RootContext.getXID());

        try {
            paymentMapper.pay(userId, credit);
        } catch (Exception e) {
            log.info("Payment exception: Seata global transaction id=================>{}", RootContext.getXID());
//            GlobalTransactionContext.reload(RootContext.getXID()).rollback();
            return false;
        }
        return true;
    }

    Lock lock = new ReentrantLock();

    @Override
    @GlobalTransactional(rollbackFor = Exception.class)
    public Boolean cancel(long userId, long orderId) throws TransactionException {
        try {
//            lock.lock();
            Order order = orderClient.findOrder(orderId);
            for (Item item : order.getItems()) {
                if (stockClient.add(item.getItemId(), item.getAmount()).equals("400")) {
                    throw new TransactionException("adding stock failed");
                }
            }
            if (!paymentMapper.add(userId, (int) order.getTotalCost())) {
                throw new TransactionException("adding credit failed");
            }
            orderClient.cancel(orderId);
        } catch (FeignException e) {
            log.error("checkout failed:{}", e.getCause(), e);
            log.info("Seata global transaction id=================>{}", RootContext.getXID());
            GlobalTransactionContext.reload(RootContext.getXID()).rollback();
            return false;
        } finally {
//            lock.unlock();
        }
        return true;
    }

    @Override
    public Boolean status(long userId, long orderId) {
        Order order = orderClient.findOrder(orderId);
        return order.isPaid();
    }

    @Override
//    @Transactional
    public Boolean add(long userId, double credit) {
        return paymentMapper.add(userId, credit);
    }

    @Override
//    @GlobalLock
//    @Transactional
    public String create() {
        long userId = YitIdHelper.nextId();
        if (paymentMapper.create(userId)) {
            return String.valueOf(userId);
        } else {
            return "400: fail to create user";
        }
    }

    @Override
//    @GlobalLock
//    @Transactional
    public Payment queryById(long userId) {
        return paymentMapper.queryById(userId);
    }
}
