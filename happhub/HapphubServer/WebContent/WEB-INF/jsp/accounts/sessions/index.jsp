<% if (request.getUserPrincipal() == null) { %>
<% response.sendRedirect("/"); %>
<% } %>
<%@ include file="/WEB-INF/jsp/header.jsp" %>
<%@ include file="body.jsp" %>
<%@ include file="/WEB-INF/jsp/footer.jsp" %>