package WDM.service.Impl;

import WDM.mapper.StockMapper;
import WDM.pojo.Stock;
import WDM.service.StockService;
import com.github.yitter.idgen.YitIdHelper;
import feign.FeignException;
import io.seata.core.context.RootContext;
import io.seata.core.exception.TransactionException;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

@Slf4j
@Service
public class StockServiceImpl implements StockService {

    @Autowired
    StockMapper stockMapper;

    @Override
//    @GlobalLock
//    @Transactional
    public Stock queryById(long itemId) {
        return stockMapper.queryById(itemId);
    }

    @Override
//    @Transactional
    public Boolean subtract(long itemId, int amount) throws TransactionException, FeignException {

        log.info("Seata global transaction id=================>{}", RootContext.getXID());
        RootContext.bind(RootContext.getXID());
        try {
            stockMapper.subtract(itemId, amount);
        } catch (Exception e) {
            log.info("Stock exception: Seata global transaction id=================>{}", RootContext.getXID());
            //seems we don't need to rollback in the branch transaction
//            GlobalTransactionContext.reload(RootContext.getXID()).rollback();
            return false;
        }
        return true;
    }

    @Override
//    @Transactional
    public Boolean add(long itemId, int amount) {
        return stockMapper.add(itemId, amount);
    }

    @Override
    @Transactional
//    @GlobalLock
    public String create(double price) {
        long itemId = YitIdHelper.nextId();// Generate random itemId using snowflake algorithm
        stockMapper.create(itemId, price);
        return String.valueOf(itemId);
    }
}
