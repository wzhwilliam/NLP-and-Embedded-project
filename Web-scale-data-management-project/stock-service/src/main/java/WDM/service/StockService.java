package WDM.service;

import WDM.pojo.Stock;
import feign.FeignException;
import io.seata.core.exception.TransactionException;

public interface StockService {
    Stock queryById(long itemId);

    Boolean subtract(long itemId, int amount) throws TransactionException, FeignException;

    Boolean add(long itemId, int amount);

    String create(double price);
}
