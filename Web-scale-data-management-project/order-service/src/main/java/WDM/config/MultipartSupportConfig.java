package WDM.config;

import feign.RequestInterceptor;
import feign.RequestTemplate;
import io.seata.core.context.RootContext;
import org.springframework.web.context.request.RequestContextHolder;
import org.springframework.web.context.request.ServletRequestAttributes;

import javax.servlet.http.HttpServletRequest;
import java.util.Enumeration;

public class MultipartSupportConfig implements RequestInterceptor {

    /**
     * Manually pass xid to the branch transaction
     *
     * @param template
     */
    @Override
    public void apply(RequestTemplate template) {
        // token in the requests headers
        ServletRequestAttributes attributes = (ServletRequestAttributes) RequestContextHolder.getRequestAttributes();
        if (attributes != null) {
            HttpServletRequest request = attributes.getRequest();
            Enumeration<String> headerNames = request.getHeaderNames();

            while (headerNames.hasMoreElements()) {
                String name = headerNames.nextElement();
                String values = request.getHeader(name);
                template.header(name, values);
            }
        }
        // xid in headers
        String xid = RootContext.getXID();
        template.header(RootContext.KEY_XID, xid);
    }

}

