package WDM.service;


import WDM.pojo.Order;
import io.seata.core.exception.TransactionException;


public interface OrderService {

    long createOrder( long userId);

    Boolean removeOrder(long orderId);

    Order findOrder( long orderId);

    Boolean addItem(long orderId,  long itemId);

    Boolean removeItem(long orderId, long itemId);

    Boolean checkout(Order order) throws TransactionException;

    void cancelOrder(long orderId);
}
