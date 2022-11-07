package WDM.service;

import WDM.pojo.Payment;
import feign.FeignException;
import io.seata.core.exception.TransactionException;


public interface PaymentService {
    Boolean pay(long id, double credit) throws FeignException, TransactionException;

    Boolean cancel(long userId, long orderId) throws TransactionException;

    Boolean status(long userid, long orderId);

    Boolean add(long id, double credit);

    String create();

    Payment queryById(long id);

}
