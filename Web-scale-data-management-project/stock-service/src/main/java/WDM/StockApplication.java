package WDM;

import com.github.yitter.contract.IdGeneratorOptions;
import com.github.yitter.idgen.YitIdHelper;
import io.seata.spring.annotation.datasource.EnableAutoDataSourceProxy;
import org.mybatis.spring.annotation.MapperScan;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;

@MapperScan("WDM.mapper")
@SpringBootApplication
@EnableAutoDataSourceProxy
public class StockApplication {
    public static void main(String[] args) {

        // Generate random id using snowflake algorithm
        IdGeneratorOptions options = new IdGeneratorOptions((short) 3);
        YitIdHelper.setIdGenerator(options);

        SpringApplication.run(StockApplication.class, args);
    }
}