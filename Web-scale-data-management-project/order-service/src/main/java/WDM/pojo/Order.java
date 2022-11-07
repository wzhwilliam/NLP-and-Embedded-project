package WDM.pojo;

import lombok.Data;

import java.util.List;

@Data
public class Order {
    private long orderId;
    private double totalCost;
    private boolean paid;
    private long userId;
    private List<Long> items;
}

